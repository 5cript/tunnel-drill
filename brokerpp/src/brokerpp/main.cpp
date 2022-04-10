#include <brokerpp/controller.hpp>
#include <brokerpp/load_home_file.hpp>
#include <brokerpp/user_control.hpp>
#include <brokerpp/config.hpp>

#include <attender/io_context/thread_pooler.hpp>
#include <attender/io_context/managed_io_context.hpp>
#include <attender/http/http_secure_server.hpp>
#include <attender/http/response.hpp>
#include <attender/http/request.hpp>
#include <attender/ssl_contexts/ssl_example_context.hpp>
#include <attender/session/basic_authorizer.hpp>
#include <spdlog/spdlog.h>

#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/traits.h>

#include <iostream>
#include <chrono>

// FIXME: this might fill up quickly
constexpr static auto IoContextThreadPoolSize = 8;

int main() 
{
    using namespace TunnelBore::Broker;

    attender::managed_io_context <attender::thread_pooler> context
    {
        static_cast <std::size_t> (IoContextThreadPoolSize),
        []()
        {
            // TODO:
        },
        [](std::exception const& exc)
        {
            std::stringstream sstr;
            sstr << std::this_thread::get_id();
            spdlog::error("Uncaught exception in thread {}: {}", sstr.str(), exc.what());
        }
    };

    UserControl userControl;

    attender::http_secure_server authServer{
        context.get_io_context(), 
        std::make_unique <attender::ssl_example_context> (getHomePath() / "key.pem", getHomePath() / "cert.pem"),
        [](auto, auto const&, auto const&){}
    };

    const auto privateJwt = loadHomeFile("jwt/private.key");
    const auto config = loadConfig();
    std::mutex authMutex;

    authServer.get("/auth", [privateJwt, &userControl, &authMutex](auto req, auto res) 
    {
        std::lock_guard lock{authMutex};
        auto authy = userControl.authenticateOnce();
        const auto authResult = authy.try_perform_authorization(req, res);
        if (authResult == attender::authorization_result::allowed_continue)
        {
            using traits = jwt::traits::nlohmann_json;
            const auto token = jwt::create<traits>()
                .set_issuer("tunnelBore")
                .set_type("JWS")
                .set_payload_claim("identity", authy.identity())
                .set_issued_at(std::chrono::system_clock::now())
                .set_expires_at(std::chrono::system_clock::now() + std::chrono::days{1})
                .sign(jwt::algorithm::rs256{"", privateJwt, "", ""})
            ;
            spdlog::info("User '{}' authenticated, sending token.", authy.identity()); 
            res->type("application/json").send(token);
        }
        else
        {
            spdlog::warn("Authentication failed with code {}", static_cast<int>(authResult));
        }
        // authentication was denied here.
    });

    // Start HTTPS authority server.
    authServer.start(std::to_string(config.authorityPort));

    auto controller = std::make_shared<Controller>(*context.get_io_context(), [](boost::system::error_code ec){
        spdlog::error("Error in websocket control line ({}).", ec.message());
    });

    // Start websocket control server.
    controller->start(config.controlPort);

    // Notify terminal user and wait.
    spdlog::info("Bound on port '{}'", config.controlPort);

    // TODO: Wait for signal in release mode, enter in debug mode.
    std::cin.get();
}