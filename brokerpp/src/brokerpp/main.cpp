#include <brokerpp/program_options.hpp>
#include <brokerpp/winsock_first.hpp>
// #include <brokerpp/controller.hpp>
#include <sharedpp/load_home_file.hpp>
#include <brokerpp/config.hpp>
#include <brokerpp/request_listener/authenticator.hpp>
#include <brokerpp/request_listener/page_control_provider.hpp>

#include <roar/server.hpp>
#include <roar/ssl/make_ssl_context.hpp>
#include <roar/utility/scope_exit.hpp>
#include <roar/utility/shutdown_barrier.hpp>
#include <roar/filesystem/special_paths.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/thread_pool.hpp>

#include <iostream>
#include <chrono>
#include <filesystem>
#include <fstream>
// Missing support for some systems:
// #include <stacktrace>

#if __linux__
#    include <signal.h>
#    include <cstdlib>
#endif

constexpr static auto IoContextThreadPoolSize = 16;

#if __linux__
void signalHandler(int signum)
{
    // auto trace = std::stacktrace::current();
    // std::cout << std::to_string(trace) << std::endl;
    std::exit(signum);
}
#endif

int main(int argc, char** argv)
{
#if __linux__
    signal(SIGSEGV, signalHandler);
#endif

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
    sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_st>(
        Roar::resolvePath("~/.tbore/broker/logs/log").string(),
        23,
        59
    ));
    auto combined_logger = std::make_shared<spdlog::logger>(
        "name",
        begin(sinks),
        end(sinks)
    );
    combined_logger->flush_on(spdlog::level::info);
    combined_logger->set_level(spdlog::level::info);
    //register it if you need to access it globally
    spdlog::register_logger(combined_logger);

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

    const auto privateJwt = loadHomeFile("broker/jwt/private.key");
    const auto publicJwt = loadHomeFile("broker/jwt/public.key");
    const auto config = loadConfig();

    if (!config.ssl)
        spdlog::warn("SSL is disabled! This is only for testing purposes!");

    Roar::Server server(
        {.executor = pool.executor(), .sslContext = [&config]() -> std::optional<boost::asio::ssl::context> {
             if (!config.ssl)
                 return std::nullopt;
             return Roar::makeSslContext(getHomePath() / "broker/cert.pem", getHomePath() / "broker/key.pem");
         }()});

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