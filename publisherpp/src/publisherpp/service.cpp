#include <publisherpp/service.hpp>

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
            boost::asio::connect(socket, endpoints);

            // create session
            auto session = std::make_shared<ServiceSession>(std::move(socket));
            session->setOnClose([weak = self->weak_from_this(), tunnelId]() {
                auto self = weak.lock();
                if (!self)
                    return;

                auto it = self->sessions_.find(tunnelId);
                if (it == self->sessions_.end())
                    return;

                if (!it->second.inward->active() && !it->second.outward->active())
                {
                    spdlog::info("Service session closed: {}", tunnelId);
                    self->sessions_.erase(it);
                }
            });
            return session;
        };

        auto outwards = createConnection(brokerHost, publicPort_);
        // write token to outwards:
        boost::asio::async_write(
            outwards->socket(),
            boost::asio::buffer(token),
            [weak = weak_from_this(), outwards, tunnelId, createConnection](boost::system::error_code ec, std::size_t) {
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
                self->sessions_.emplace(tunnelId, ServiceSessionPair{inwards, outwards});
                inwards->pipeTo(*outwards);
                outwards->pipeTo(*inwards);
            });
    }
}