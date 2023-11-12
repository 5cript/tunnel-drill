#include <publisherpp/service.hpp>

#include <sharedpp/constants.hpp>

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

namespace TunnelBore::Publisher
{
    Service::Service(
        boost::asio::any_io_executor executor,
        std::optional<std::string> name,
        int publicPort,
        std::string hiddenHost,
        int hiddenPort)
        : executor_{std::move(executor)}
        , name_{std::move(name)}
        , publicPort_{publicPort}
        , hiddenHost_{std::move(hiddenHost)}
        , hiddenPort_{hiddenPort}
        , sessions_{}
    {}
    std::string Service::name() const
    {
        if (name_)
            return name_.value();
        return hiddenHost_ + ":" + std::to_string(hiddenPort_);
    }
    int Service::publicPort() const
    {
        return publicPort_;
    }
    std::string const& Service::hiddenHost() const
    {
        return hiddenHost_;
    }
    int Service::hiddenPort() const
    {
        return hiddenPort_;
    }
    void Service::createSession(std::string const& brokerHost, std::string const& token, std::string const& tunnelId)
    {
        auto createConnection = [weak = weak_from_this(), tunnelId](std::string const& host, int port) {
            auto self = weak.lock();
            if (!self)
            {
                spdlog::error("Service::createSession: weak.lock() failed");
                return std::shared_ptr<ServiceSession>{};
            }

            // resolve remote
            auto resolver = boost::asio::ip::tcp::resolver{self->executor_};
            auto endpoints = resolver.resolve(host, std::to_string(port));

            // create socket synchronously
            auto socket = boost::asio::ip::tcp::socket{self->executor_};
            boost::system::error_code ec;
            boost::asio::connect(socket, endpoints, ec);
            if (ec)
            {
                spdlog::error("Service::createSession: connect failed: {}", ec.message());
                return std::shared_ptr<ServiceSession>{};
            }

            // create session
            auto session = std::make_shared<ServiceSession>(std::move(socket));
            session->setOnClose([weak = self->weak_from_this(), tunnelId]() {
                auto self = weak.lock();
                if (!self)
                    return;

                std::scoped_lock lock{self->sessionGuard_};

                auto it = self->sessions_.find(tunnelId);
                if (it == self->sessions_.end())
                    return;

                if (!it->second.inward->active() && !it->second.outward->active())
                {
                    spdlog::info("Service session closed: {}", tunnelId);
                    auto& session = it->second;
                    session.inwardPipe->close();
                    session.outwardPipe->close();
                    self->sessions_.erase(it);
                }
            });
            return session;
        };

        auto prefixedToken = std::make_shared<std::string>(std::string{publisherToBrokerPrefix} + ":" + token);
        auto outwards = createConnection(brokerHost, publicPort_);
        if (!outwards)
            return;
        // write token to outwards:
        boost::asio::async_write(
            outwards->socket(),
            boost::asio::buffer(*prefixedToken),
            [weak = weak_from_this(), outwards, tunnelId, createConnection, prefixedToken](
                boost::system::error_code ec, std::size_t) {
                if (ec)
                {
                    spdlog::error("Failed to write token to outwards connection: {}", ec.message());
                    return;
                }

                auto self = weak.lock();
                if (!self)
                {
                    spdlog::error("Failed to write token to outwards connection: Service is dead");
                    return;
                }

                auto inwards = createConnection(self->hiddenHost_, self->hiddenPort_);
                if (!inwards)
                {
                    outwards->close();
                    return;
                }
                {
                    std::scoped_lock lock{self->sessionGuard_};
                    auto elem = self->sessions_.emplace(tunnelId, ServiceSessionPair{inwards, outwards, {}, {}});

                    spdlog::info("Connecting pipes");
                    elem.first->second.inwardPipe = inwards->pipeTo(*outwards);
                    elem.first->second.outwardPipe = outwards->pipeTo(*inwards);
                }
            });
    }
}