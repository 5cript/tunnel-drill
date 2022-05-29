#pragma once

#include <roar/routing/request_listener.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/describe/class.hpp>

#include <memory>
#include <string>

using namespace Roar::Literals;

namespace TunnelBore::Broker
{
    class Publisher;

    class PageAndControlProvider : public std::enable_shared_from_this<PageAndControlProvider>
    {
      public:
        PageAndControlProvider(
            boost::asio::any_io_executor executor,
            std::string publicJwt,
            std::filesystem::path directory);
        ROAR_PIMPL_SPECIAL_FUNCTIONS(PageAndControlProvider);

        std::shared_ptr<Publisher> obtainPublisher(std::string const& identity);
        std::filesystem::path getServedDirectory() const;

      private:
        ROAR_MAKE_LISTENER(PageAndControlProvider);

        ROAR_GET(status)
        ({
            .path = "/api/status",
        });
        ROAR_SERVE(serve)
        (Roar::ServeInfo<PageAndControlProvider>{
            .path = "/",
            .routeOptions = {.allowUnsecure = false},
            .serveOptions = {
                .allowDownload = true,
                .allowListing = false,
                .pathProvider = &PageAndControlProvider::getServedDirectory,
            }});
        ROAR_GET(publisher)
        ({
            .path = "/api/ws/publisher",
            .routeOptions = {.expectUpgrade = true},
        });
        ROAR_GET(redirect1)("/login");
        ROAR_GET(redirect2)("/stats/.*"_rgx);
        ROAR_GET(redirect3)("/settings/.*"_rgx);

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;

      private:
        BOOST_DESCRIBE_CLASS(
            PageAndControlProvider,
            (),
            (),
            (),
            (roar_serve, roar_publisher, roar_status, roar_redirect1, roar_redirect2, roar_redirect3));
    };
}