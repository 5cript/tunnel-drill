#pragma once

#include <brokerpp/authority.hpp>
#include <brokerpp/user_control.hpp>

#include <roar/routing/request_listener.hpp>
#include <boost/describe/class.hpp>

#include <mutex>
#include <memory>

namespace TunnelBore::Broker
{
    class Authenticator : public std::enable_shared_from_this<Authenticator>
    {
      public:
        Authenticator(std::shared_ptr<Authority> authority);

      private:
        ROAR_MAKE_LISTENER(Authenticator);

        ROAR_GET(auth)("/api/auth");
        ROAR_POST(signJson)("/api/auth/sign-json");

      private:
        std::shared_ptr<Authority> authority_;

      private:
        BOOST_DESCRIBE_CLASS(Authenticator, (), (), (), (roar_auth, roar_signJson));
    };
}