#pragma once

#include <brokerpp/json.hpp>
#include <attender/websocket/server/flexible_session.hpp>
#include <memory>

namespace TunnelBore::Broker
{
    class Controller;

    class ControlSession
        : public attender::websocket::session_base
        , public std::enable_shared_from_this<ControlSession>
    {
    public:
        ControlSession(
            attender::websocket::connection* owner,
            std::string sessionId
        );

        ~ControlSession();
        ControlSession(ControlSession&&) = delete;
        ControlSession(ControlSession const&) = delete;
        ControlSession& operator=(ControlSession&&) = delete;
        ControlSession& operator=(ControlSession const&) = delete;

        void setup();

        void on_close() override;
        void on_text(std::string_view) override;
        void on_binary(char const*, std::size_t) override;
        void on_error(boost::system::error_code, char const*) override;
        void on_write_complete(std::size_t) override;

        bool writeJson(json const& j, std::function<void(session_base*, std::size_t)> const& on_complete = {});
        bool writeText(std::string const& txt, std::function<void(session_base*, std::size_t)> const& on_complete = {});
        void onJson(json const& j, std::string const& ref);
        void respondWithError(int ref, std::string const& msg);

    private:
        void onAfterAuthentication();
        void endSession();

    private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}