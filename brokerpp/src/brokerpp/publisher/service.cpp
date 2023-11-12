
#include <boost/asio/ip/tcp.hpp>

#include <brokerpp/publisher/service.hpp>
#include <brokerpp/publisher/publisher.hpp>
#include <brokerpp/publisher/tunnel_session.hpp>
#include <sharedpp/uuid_generator.hpp>
#include <spdlog/spdlog.h>

#include <roar/utility/scope_exit.hpp>

#include <mutex>
#include <shared_mutex>

namespace leaf = boost::leaf;

namespace TunnelBore::Broker
{
    // #####################################################################################################################
    struct Service::Implementation
    {
        boost::asio::ip::tcp::acceptor acceptor;
        std::shared_mutex acceptorStopGuard;
        std::unordered_map<std::string, std::shared_ptr<TunnelSession>> sessions;
        ServiceInfo info;
        boost::asio::ip::tcp::endpoint bindEndpoint;
        std::weak_ptr<Publisher> publisher;
        uuid_generator uuidGenerator;
        std::recursive_mutex sessionGuard;
        std::string serviceId;

        Implementation(
            boost::asio::any_io_executor executor,
            ServiceInfo const& info,
            boost::asio::ip::tcp::endpoint bindEndpoint,
            std::weak_ptr<Publisher> publisher,
            std::string serviceId)
            : acceptor{std::move(executor)}
            , acceptorStopGuard{}
            , sessions{}
            , info{info}
            , bindEndpoint{bindEndpoint}
            , publisher{std::move(publisher)}
            , uuidGenerator{}
            , sessionGuard{}
            , serviceId{std::move(serviceId)}
        {}
    };
    // #####################################################################################################################
    Service::Service(
        boost::asio::any_io_executor executor,
        ServiceInfo const& info,
        boost::asio::ip::basic_endpoint<boost::asio::ip::tcp> const& bindEndpoint,
        std::weak_ptr<Publisher> publisher,
        std::string serviceId)
        : impl_{std::make_unique<Implementation>(
              std::move(executor),
              info,
              bindEndpoint,
              std::move(publisher),
              std::move(serviceId))}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Service::~Service()
    {
        spdlog::info("Service '{}' is being destroyed.", impl_->serviceId);
        stop();
    }
    //---------------------------------------------------------------------------------------------------------------------
    ServiceInfo Service::info() const
    {
        return impl_->info;
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::string Service::serviceId() const
    {
        return impl_->serviceId;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Service::connectTunnels(std::string const& idForClientTunnel, std::string const& idForPublisherTunnel)
    {
        std::scoped_lock lock{impl_->sessionGuard};
        auto clientTunnel = impl_->sessions.find(idForClientTunnel);
        auto publisherTunnel = impl_->sessions.find(idForPublisherTunnel);

        if (clientTunnel == std::end(impl_->sessions))
        {
            spdlog::error("Tunnel link up failed, because the client tunnnel side is gone.");
            closeTunnelSide(idForPublisherTunnel);
            return;
        }
        if (publisherTunnel == std::end(impl_->sessions))
        {
            spdlog::error("Tunnel link up failed, because the publisher tunnel side is gone.");
            closeTunnelSide(idForClientTunnel);
            return;
        }

        spdlog::info("Linking tunnels '{}' and '{}'.", idForClientTunnel, idForPublisherTunnel);
        clientTunnel->second->link(*publisherTunnel->second);
    }
    //---------------------------------------------------------------------------------------------------------------------
    Service::Service(Service&&) = default;
    //---------------------------------------------------------------------------------------------------------------------
    Service& Service::operator=(Service&&) = default;
    //---------------------------------------------------------------------------------------------------------------------
    boost::leaf::result<void> Service::start()
    {
        stop();
        boost::system::error_code ec;

        impl_->acceptor.open(impl_->bindEndpoint.protocol(), ec);
        if (ec)
            return leaf::new_error("Could not open http server acceptor.", ec);

        impl_->acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec)
            return leaf::new_error("Could not configure socket to reuse address.", ec);

        impl_->acceptor.bind(impl_->bindEndpoint, ec);
        if (ec)
            return leaf::new_error("Could not bind socket.", ec);

        impl_->acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec)
            return leaf::new_error("Could not listen on socket.", ec);

        acceptOnce();
        return {};
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Service::closeTunnelSide(std::string const& id)
    {
        std::scoped_lock lock{impl_->sessionGuard};
        spdlog::info("[Service '{}']: Closing tunnel side '{}'.", impl_->serviceId, id);
        impl_->sessions.erase(id);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Service::acceptOnce()
    {
        spdlog::info("[Service '{}']: Accepting connection.", impl_->serviceId);

        std::shared_ptr<boost::asio::ip::tcp::socket> socket =
            std::make_shared<boost::asio::ip::tcp::socket>(impl_->acceptor.get_executor());

        impl_->acceptor.async_accept(*socket, [weak = weak_from_this(), socket](boost::system::error_code ec) mutable {
            const auto acceptorExitLog = Roar::ScopeExit{[weak]() {
                auto self = weak.lock();
                if (!self)
                    return;
                spdlog::info("[Service '{}']: Accepting connection finished.", self->impl_->serviceId);
            }};

            auto self = weak.lock();
            if (!self)
            {
                spdlog::warn("Service is gone, cannot accept new connections.");
                return;
            }

            if (ec == boost::asio::error::operation_aborted)
            {
                spdlog::info("[Service '{}']: Acceptor was stopped.", self->impl_->serviceId);
                return;
            }

            if (ec)
            {
                spdlog::error("[Service '{}']: Could not accept connection: {}", self->impl_->serviceId, ec.message());
                return self->acceptOnce();
            }

            spdlog::info("[Service '{}']: Accepted connection.", self->impl_->serviceId);
            std::shared_lock lock{self->impl_->acceptorStopGuard};

            spdlog::info(
                "[Service '{}']: Acceptor is open: {}", self->impl_->serviceId, self->impl_->acceptor.is_open());
            if (!self->impl_->acceptor.is_open())
                return;

            auto publisher = self->impl_->publisher.lock();
            if (!publisher)
            {
                spdlog::warn(
                    "[Service '{}']: Publisher is gone, cannot accept new connections.", self->impl_->serviceId);
                return;
            }

            // cannot accept tunnels, if we cannot communicate with the publisher.
            auto controlSession = publisher->getCurrentControlSession().lock();
            if (!controlSession)
            {
                spdlog::warn(
                    "[Service '{}']: Control session is gone, cannot accept new connections.", self->impl_->serviceId);
                return;
            }

            if (!socket->is_open())
            {
                spdlog::warn(
                    "[Service '{}']: Socket is not open, but acceptor did not return an error.",
                    self->impl_->serviceId);
                return self->acceptOnce();
            }

            const auto tunnelId = self->impl_->uuidGenerator.generate_id();
            spdlog::info(
                "[Service '{}']: New connection accepted '{}' with tunnelId '{}'.",
                self->impl_->serviceId,
                socket->remote_endpoint(ec).address().to_string(),
                tunnelId);
            {
                std::scoped_lock sessionLock{self->impl_->sessionGuard};
                auto tunnelSide = std::make_shared<TunnelSession>(
                    std::move(*socket), tunnelId, controlSession, self->weak_from_this());
                self->impl_->sessions[tunnelId] = std::move(tunnelSide);
                self->impl_->sessions[tunnelId]->peek();
            }

            self->acceptOnce();
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Service::stop()
    {
        std::scoped_lock lock{impl_->acceptorStopGuard};
        spdlog::info("Stopping service '{}' acceptor.", impl_->serviceId);
        impl_->acceptor.close();
    }
    // #####################################################################################################################
}