#pragma once

#include <brokerpp/publisher/publisher.hpp>

#include <attender/websocket/server/server.hpp>
#include <attender/session/uuid_session_cookie_generator.hpp>

#include <memory>
#include <unordered_map>
#include <mutex>

namespace TunnelBore::Broker
{

    class Controller : public std::enable_shared_from_this<Controller>
    {
      public:
        Controller(boost::asio::io_context& context, std::function<void(boost::system::error_code)> on_error);
        ~Controller();

        void start(unsigned short port);
        void stop();

        std::shared_ptr<Publisher> obtainPublisher(std::string const& identity);

      private:
        boost::asio::io_context* context_;
        attender::websocket::server ws_;
        std::mutex guard_;
        std::unordered_map<std::string, std::shared_ptr<attender::websocket::connection>> connections_;
        attender::uuid_generator generator_;

        std::unordered_map<std::string, std::shared_ptr<Publisher>> publishers_;
    };

}