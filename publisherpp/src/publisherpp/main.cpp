#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/thread_pool.hpp>

#include <publisherpp/publisher.hpp>

#include <sharedpp/load_home_file.hpp>
#include <roar/utility/scope_exit.hpp>
#include <roar/utility/shutdown_barrier.hpp>
#include <spdlog/spdlog.h>

#include <iostream>

constexpr static auto IoContextThreadPoolSize = 16;

int main()
{
    using namespace TunnelBore;
    using namespace TunnelBore::Publisher;
    using namespace std::string_literals;

    setupHome();

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
    //Roar::shutdownBarrier.wait();
    std::cin.get();

    spdlog::info("Shutting down...");
}