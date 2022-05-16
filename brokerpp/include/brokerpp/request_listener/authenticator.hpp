#pragma once

#include <brokerpp/user_control.hpp>

#include <roar/routing/request_listener.hpp>
#include <boost/describe/class.hpp>

#include <mutex>

namespace TunnelBore::Broker
{
    class Authenticator
    {
      public:
        Authenticator(std::string privateJwt);

      private:
        ROAR_MAKE_LISTENER(Authenticator);

        ROAR_GET(auth)("/auth");

      private:
        std::mutex authMutex_;
        std::string privateJwt_;
        UserControl userControl_;

      private:
        BOOST_DESCRIBE_CLASS(Authenticator, (), (), (), (roar_auth));
    };
}