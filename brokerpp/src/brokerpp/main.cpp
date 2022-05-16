#include <brokerpp/winsock_first.hpp>
#include <brokerpp/controller.hpp>
#include <brokerpp/load_home_file.hpp>
#include <brokerpp/config.hpp>
#include <brokerpp/request_listener/authenticator.hpp>
#include <brokerpp/request_listener/page_control_provider.hpp>

#include <roar/server.hpp>
#include <roar/ssl/make_ssl_context.hpp>

#include <spdlog/spdlog.h>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/thread_pool.hpp>

#include <iostream>
#include <chrono>
#include <filesystem>
#include <fstream>

// FIXME: this might fill up quickly
constexpr static auto IoContextThreadPoolSize = 8;

int main()
{
    using namespace TunnelBore::Broker;
    using namespace std::string_literals;

    boost::asio::thread_pool pool{IoContextThreadPoolSize};

    Roar::Server server(
        {.executor = pool.executor(),
         .sslContext = Roar::makeSslContext({
             .certificate = getHomePath() / "cert.pem",
             .privateKey = getHomePath() / "key.pem",
         })});

    const auto privateJwt = loadHomeFile("jwt/private.key");
    const auto publicJwt = loadHomeFile("jwt/public.key");
    const auto config = loadConfig();

    server.installRequestListener<Authenticator>(privateJwt);
    server.installRequestListener<PageAndControlProvider>(pool.executor(), publicJwt);

    // auto controller = std::make_shared<Controller>(*context.get_io_context(), [](boost::system::error_code ec) {
    //     spdlog::error("Error in websocket control line ({}).", ec.message());
    // });

    // Start websocket control server.
    // controller->start(config.controlPort);

    // Notify terminal user and wait.
    spdlog::info("Bound on port '{}'", config.controlPort);

    // TODO: Wait for signal in release mode, enter in debug mode.
    std::cin.get();
}