#include <brokerpp/program_options.hpp>
#include <brokerpp/winsock_first.hpp>
#include <brokerpp/controller.hpp>
#include <sharedpp/load_home_file.hpp>
#include <brokerpp/config.hpp>
#include <brokerpp/request_listener/authenticator.hpp>
#include <brokerpp/request_listener/page_control_provider.hpp>

#include <roar/server.hpp>
#include <roar/ssl/make_ssl_context.hpp>

#include <spdlog/spdlog.h>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/thread_pool.hpp>
#include <roar/utility/scope_exit.hpp>
#include <roar/utility/shutdown_barrier.hpp>

#include <iostream>
#include <chrono>
#include <filesystem>
#include <fstream>

constexpr static auto IoContextThreadPoolSize = 16;

int main(int argc, char** argv)
{
    using namespace TunnelBore;
    using namespace TunnelBore::Broker;
    using namespace std::string_literals;

    const auto programOptions = parseProgramOptions(argc, argv);

    spdlog::info("Serving documents from: {}", programOptions.servedDirectory);

    setupHome();

    boost::asio::thread_pool pool{IoContextThreadPoolSize};
    const auto shutdownPool = Roar::ScopeExit{[&pool]() {
        pool.stop();
        pool.join();
    }};

    spdlog::info("Config files are at '{}'", getHomePath().string());

    Roar::Server server(
        {.executor = pool.executor(),
         .sslContext = Roar::makeSslContext({
             .certificate = getHomePath() / "cert.pem",
             .privateKey = getHomePath() / "key.pem",
         })});

    const auto privateJwt = loadHomeFile("broker/jwt/private.key");
    const auto publicJwt = loadHomeFile("broker/jwt/public.key");
    const auto config = loadConfig();

    auto authority = std::make_shared<Authority>(privateJwt);

    server.installRequestListener<Authenticator>(authority);
    server.installRequestListener<PageAndControlProvider>(pool.executor(), publicJwt, programOptions.servedDirectory);

    server.start(config.bind.port, config.bind.iface);

    // Notify terminal user and wait.
    spdlog::info("Bound on [{}]:'{}'", config.bind.iface, server.getLocalEndpoint().port());

    // Wait for signal:
    Roar::shutdownBarrier.wait();

    spdlog::info("Shutting down...");
}