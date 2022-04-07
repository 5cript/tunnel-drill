#pragma once

#include "service_info.hpp"

#include <boost/leaf.hpp>

#include <memory>

namespace boost::asio
{
    class io_context;
}
namespace boost::asio::ip
{
    class tcp;
    template <typename T>
    class basic_endpoint;
}

namespace TunnelBore::Broker
{

class Service : public std::enable_shared_from_this<Service>
{
public:
    Service(
        boost::asio::io_context& context, 
        ServiceInfo const& info, 
        boost::asio::ip::basic_endpoint<boost::asio::ip::tcp> const& bindEndpoint
    );
    ~Service();
    Service(Service const&) = delete;
    Service(Service&&);
    Service& operator=(Service const&) = delete;
    Service& operator=(Service&&);

    boost::leaf::result<void> start();
    void stop();

private:
    void acceptOnce();

private:
    struct Implementation;
    std::unique_ptr <Implementation> impl_;
};

}