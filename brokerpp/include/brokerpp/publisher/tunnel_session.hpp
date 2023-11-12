#pragma once

#include <sharedpp/pipe_operation.hpp>
#include <brokerpp/authority.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <memory>
#include <string>
#include <chrono>

namespace TunnelBore::Broker
{
    class ControlSession;
    class Service;

    class TunnelSession : public std::enable_shared_from_this<TunnelSession>
    {
      public:
        constexpr static std::chrono::seconds InactivityTimeout{30};

        TunnelSession(
            boost::asio::ip::tcp::socket&& socket,
            std::string tunnelId,
            std::weak_ptr<ControlSession> controlSession,
            std::weak_ptr<Service> service);
        ~TunnelSession();
        TunnelSession(TunnelSession const&) = delete;
        TunnelSession(TunnelSession&&);
        TunnelSession& operator=(TunnelSession const&) = delete;
        TunnelSession& operator=(TunnelSession&&);

        void close();
        void link(TunnelSession& other);
        void peek();
        [[nodiscard]] std::shared_ptr<PipeOperation<TunnelSession>> pipeTo(TunnelSession& other);
        boost::asio::ip::tcp::socket& socket();
        std::string id() const;
        std::string remoteAddress() const;
        std::recursive_mutex& closeGuard();

        void resetTimer();
        void cancelTimer();

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}