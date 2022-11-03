
#include <boost/asio/ip/tcp.hpp>

#include <brokerpp/publisher/service.hpp>
#include <brokerpp/publisher/publisher.hpp>
#include <brokerpp/publisher/tunnel_session.hpp>
#include <attender/session/uuid_session_cookie_generator.hpp>
#include <spdlog/spdlog.h>

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
        attender::uuid_generator uuidGenerator;
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
        impl_->sessions.erase(id);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Service::acceptOnce()
    {
        std::shared_ptr<boost::asio::ip::tcp::socket> socket =
            std::make_shared<boost::asio::ip::tcp::socket>(impl_->acceptor.get_executor());

        impl_->acceptor.async_accept(
            *socket, [self = shared_from_this(), socket](boost::system::error_code ec) mutable {
                if (ec == boost::asio::error::operation_aborted)
                    return;

                std::shared_lock lock{self->impl_->acceptorStopGuard};

                if (!self->impl_->acceptor.is_open())
                    return;

                auto publisher = self->impl_->publisher.lock();
                if (!publisher)
                    return;

                // cannot accept tunnels, if we cannot communicate with the publisher.
                auto controlSession = publisher->getCurrentControlSession().lock();
                if (!controlSession)
                    return;

                if (ec)
                    return self->acceptOnce();

                const auto tunnelId = self->impl_->uuidGenerator.generate_id();
                spdlog::info(
                    "New connection accepted '{}' with tunnelId '{}'.", 
                    socket->remote_endpoint(ec).address().to_string(), 
                    tunnelId
                );
                {
                    std::scoped_lock sessionLock{self->impl_->sessionGuard};
                    auto tunnelSide =
                        std::make_shared<TunnelSession>(std::move(*socket), tunnelId, controlSession, self);
                    tunnelSide->peek();
                    self->impl_->sessions[tunnelId] = std::move(tunnelSide);
                }

                self->acceptOnce();
            });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Service::stop()
    {
        std::scoped_lock lock{impl_->acceptorStopGuard};
        impl_->acceptor.close();
    }
    // #####################################################################################################################
}