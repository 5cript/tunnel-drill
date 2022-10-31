#pragma once

#include <boost/asio/ip/tcp.hpp>

#include <memory>
#include <string>

namespace TunnelBore::Publisher
{
    class ServiceSession : public std::enable_shared_from_this<ServiceSession>
    {
      public:
        ServiceSession(boost::asio::ip::tcp::socket&& socket);
        ~ServiceSession();
        ServiceSession(ServiceSession const&) = delete;
        ServiceSession(ServiceSession&&);
        ServiceSession& operator=(ServiceSession const&) = delete;
        ServiceSession& operator=(ServiceSession&&);
        void setOnClose(std::function<void()> onClose);

        void close();
        void resetTimer();
        boost::asio::ip::tcp::socket& socket();
        std::string remoteAddress();
        void pipeTo(ServiceSession& other);
        bool active();

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}