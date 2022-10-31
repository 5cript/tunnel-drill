#pragma once

#include <brokerpp/user_control.hpp>
#include <sharedpp/json.hpp>

#include <string>
#include <mutex>

namespace TunnelBore::Broker
{
    class Authority
    {
      public:
        Authority(std::string privateJwt);

        std::optional<std::string>
        authenticateThenSign(std::string const& name, std::string const& password, json const& claims = {});

      private:
        std::mutex authMutex_;
        std::string privateJwt_;
        UserControl userControl_;
    };
}