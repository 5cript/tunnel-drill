#pragma once

#include <publisherpp/config.hpp>
#include <publisherpp/service.hpp>
#include <roar/websocket/websocket_client.hpp>
#include <roar/websocket/read_result.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <chrono>
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <mutex>

namespace TunnelBore::Publisher
{
    struct ControlSendOperation
    {
        json payload;
        std::function<void()> callback;
    };

    class Publisher : public std::enable_shared_from_this<Publisher>
    {
      public:
        Publisher(boost::asio::any_io_executor exec, Config cfg);
        ~Publisher();
        Publisher(Publisher const&) = delete;
        Publisher& operator=(Publisher const&) = delete;
        Publisher(Publisher&&) = delete;
        Publisher& operator=(Publisher&&) = delete;

        void authenticate();

      private:
        void connectToBroker();
        void retryConnect();
        void doControlReading();
        void onControlRead(Roar::WebsocketReadResult message);
        void sendQueued(json&& j);
        void sendOnceFromQueue();
        void onNewTunnel(
            std::string const& serviceId,
            std::string const& tunnelId,
            int hiddenPort,
            int publicPort,
            std::string const& socketType);
        static std::shared_ptr<Roar::WebsocketClient> createWebsocketClient(boost::asio::any_io_executor exec, Config const& cfg);

      private:
        const Config cfg_;
        boost::asio::any_io_executor exec_;
        std::shared_ptr<Roar::WebsocketClient> ws_;
        std::vector<std::shared_ptr<Service>> services_;
        std::string authToken_;
        std::chrono::system_clock::time_point tokenCreationTime_;
        
        // reconnect related
        boost::asio::deadline_timer reconnectTimer_;
        std::chrono::seconds reconnectTime_;
        bool isReconnecting_;
        std::recursive_mutex reconnectMutex_;

        // send queue related
        std::recursive_mutex controlSendQueueMutex_;
        std::deque<ControlSendOperation> controlSendOperations_;
        bool sendInProgress_;
    };
}