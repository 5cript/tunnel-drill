#pragma once

#include <brokerpp/json.hpp>

namespace TunnelBore::Broker
{
    class User
    {
      public:
        bool authenticate(json const& payload);
    };
}