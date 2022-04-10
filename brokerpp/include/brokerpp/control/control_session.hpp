#pragma once

#include <brokerpp/json.hpp>
#include <brokerpp/control/subscription.hpp>

#include <attender/websocket/server/flexible_session.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <memory>

namespace TunnelBore::Broker
{
    class Controller;
    class Publisher;

    class ControlSession
        : public attender::websocket::session_base
        , public std::enable_shared_from_this<ControlSession>
    {
    public:
        ControlSession(
            attender::websocket::connection* owner,
            std::weak_ptr<Controller> controller,
            std::string sessionId
        );

        ~ControlSession();
        ControlSession(ControlSession&&) = delete;
        ControlSession(ControlSession const&) = delete;
        ControlSession& operator=(ControlSession&&) = delete;
        ControlSession& operator=(ControlSession const&) = delete;

        std::string identity() const;
        boost::asio::ip::tcp::endpoint remoteEndpoint() const;

        void subscribe(
            std::string const& type, 
            std::function<bool(Subscription::ParameterType const&, std::string const&)> const& callback
        );

        void informAboutConnection(std::string const& serviceId, std::string const& tunnelId);

        void setup(std::string const& identity);

        void on_close() override;
        void on_text(std::string_view) override;
        void on_binary(char const*, std::size_t) override;
        void on_error(boost::system::error_code, char const*) override;
        void on_write_complete(std::size_t) override;

        bool writeJson(json const& j, std::function<void(session_base*, std::size_t)> const& on_complete = {});
        bool writeText(std::string const& txt, std::function<void(session_base*, std::size_t)> const& on_complete = {});
        void onJson(json const& j, std::string const& ref);
        void respondWithError(std::string const& ref, std::string const& msg);

    private:
        void onAfterAuthentication();
        void endSession();
        std::shared_ptr<Publisher> getAssociatedPublisher();

    private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}