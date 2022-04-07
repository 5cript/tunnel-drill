#pragma once

#include <boost/asio/ip/tcp.hpp>

namespace TunnelBore::Broker
{
    class Session
    {
    public:
        Session(boost::asio::ip::tcp::socket&& socket);
        ~Session();
        Session(Session const&) = delete;
        Session(Session&&);
        Session& operator=(Session const&) = delete;
        Session& operator=(Session&&);

        void close();

    private:
        struct Implementation;
        std::unique_ptr <Implementation> impl_;
    };
}