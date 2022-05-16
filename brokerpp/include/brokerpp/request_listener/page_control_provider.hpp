#pragma once

#include <roar/routing/request_listener.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/describe/class.hpp>

#include <memory>
#include <string>

namespace TunnelBore::Broker
{
    class Publisher;

    class PageAndControlProvider : public std::enable_shared_from_this<PageAndControlProvider>
    {
      public:
        PageAndControlProvider(boost::asio::any_io_executor executor, std::string publicJwt);
        ROAR_PIMPL_SPECIAL_FUNCTIONS(PageAndControlProvider);

        std::shared_ptr<Publisher> obtainPublisher(std::string const& identity);

      private:
        ROAR_MAKE_LISTENER(PageAndControlProvider);

        ROAR_GET(index)("/");
        ROAR_GET(publisher)
        ({
            .path = "/api/ws/publisher",
            .routeOptions = {.expectUpgrade = true},
        });

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;

      private:
        BOOST_DESCRIBE_CLASS(PageAndControlProvider, (), (), (), (roar_index, roar_publisher));
    };
}