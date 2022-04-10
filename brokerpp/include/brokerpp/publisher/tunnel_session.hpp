#pragma once

#include <boost/asio/ip/tcp.hpp>

#include <memory>
#include <string>

namespace TunnelBore::Broker
{
    class ControlSession;
    class Service;

    class TunnelSession : public std::enable_shared_from_this<TunnelSession>
    {
    public:
        TunnelSession(
            boost::asio::ip::tcp::socket&& socket, 
            std::string tunnelId, 
            std::weak_ptr<ControlSession> controlSession,
            std::weak_ptr<Service> service
        );
        ~TunnelSession();
        TunnelSession(TunnelSession const&) = delete;
        TunnelSession(TunnelSession&&);
        TunnelSession& operator=(TunnelSession const&) = delete;
        TunnelSession& operator=(TunnelSession&&);

        void close();
        void link(TunnelSession& other);

    private:
        void peek();

    private:
        struct Implementation;
        std::unique_ptr <Implementation> impl_;
    };
}