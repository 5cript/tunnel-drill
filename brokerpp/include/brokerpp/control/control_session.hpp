#pragma once

#include <brokerpp/publisher/publisher_token.hpp>
#include <brokerpp/json.hpp>
#include <brokerpp/control/subscription.hpp>
#include <roar/websocket/websocket_session.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <memory>

namespace TunnelBore::Broker
{
    class PageAndControlProvider;
    class Publisher;

    class ControlSession : public std::enable_shared_from_this<ControlSession>
    {
      public:
        ControlSession(
            std::string sessionId,
            std::weak_ptr<PageAndControlProvider> controller,
            std::shared_ptr<Roar::WebsocketSession> ws,
            std::function<void()> endSelf,
            std::string publicJwtKey);
        ~ControlSession();
        ControlSession(ControlSession&&) = delete;
        ControlSession(ControlSession const&) = delete;
        ControlSession& operator=(ControlSession&&) = delete;
        ControlSession& operator=(ControlSession const&) = delete;
        std::optional<PublisherToken> verifyPublisherIdentity(std::string const& token) const;

        Roar::Detail::PromiseTypeBind<
            Roar::Detail::PromiseTypeBindThen<std::size_t>,
            Roar::Detail::PromiseTypeBindFail<Roar::Error const&>>
        writeJson(json const& j);
        std::string identity() const;

        // TODO: still right approach?
        void setup(std::string const& identity);
        void informAboutConnection(std::string const& serviceId, std::string const& tunnelId);

        void subscribe(
            std::string const& type,
            std::function<bool(Subscription::ParameterType const&, std::string const&)> const& callback);
        boost::asio::ip::tcp::endpoint remoteEndpoint() const;
        void respondWithError(std::string const& ref, std::string const& msg);

      private:
        void onRead(Roar::WebsocketReadResult const& readResult);
        void doRead();
        void onJson(json const& j, std::string const& ref);
        std::shared_ptr<Publisher> getAssociatedPublisher();

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}