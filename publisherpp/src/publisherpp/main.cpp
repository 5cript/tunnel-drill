#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/thread_pool.hpp>

#include <publisherpp/publisher.hpp>

#include <sharedpp/load_home_file.hpp>
#include <roar/utility/scope_exit.hpp>
#include <roar/utility/shutdown_barrier.hpp>
#include <roar/filesystem/special_paths.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#include <iostream>

constexpr static auto IoContextThreadPoolSize = 16;

int main()
{
    {
        using namespace TunnelBore;
        using namespace TunnelBore::Publisher;
        using namespace std::string_literals;

        setupHome();

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
        sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_st>(
            Roar::resolvePath("~/.tbore/publisher/logs/log").string(), 23, 59));
        auto combined_logger = std::make_shared<spdlog::logger>("name", begin(sinks), end(sinks));
        spdlog::register_logger(combined_logger);
        spdlog::flush_on(spdlog::level::warn);
        spdlog::set_level(spdlog::level::info);
        spdlog::flush_every(std::chrono::seconds(10));

        boost::asio::thread_pool pool{IoContextThreadPoolSize};
        const auto shutdownPool = Roar::ScopeExit{[&pool]() {
            pool.stop();
            pool.join();
        }};

        spdlog::info("Config files are at '{}'", getHomePath().string());

        const auto config = loadConfig();

        if (!config.ssl)
            spdlog::warn("SSL is disabled! This is only for testing purposes!");

        auto publisher = std::make_shared<class Publisher>(pool.executor(), config);
        publisher->authenticate();

        // Wait for signal:
        Roar::shutdownBarrier.wait();

        spdlog::info("Shutting down...");
    }
    spdlog::shutdown();
}