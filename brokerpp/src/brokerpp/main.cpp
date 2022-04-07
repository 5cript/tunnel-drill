//#include <brokerpp/connection_broker.hpp>

#include <attender/io_context/thread_pooler.hpp>
#include <attender/io_context/managed_io_context.hpp>
#include <spdlog/spdlog.h>

#include <iostream>

// FIXME: this might fill up quickly
constexpr static auto IoContextThreadPoolSize = 8;

int main() 
{
  //using namespace TunnelBore::Broker;

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

  //ConnectionAcceptor broker{*context.get_io_context()};
}