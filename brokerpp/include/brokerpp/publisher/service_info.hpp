#pragma once

#include <brokerpp/json.hpp>

#include <optional>
#include <string>

namespace TunnelBore::Broker
{
    struct ServiceInfo
    {
        std::optional <std::string> name;
        unsigned short publicPort;
        unsigned short hiddenPort;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ServiceInfo, name, publicPort, hiddenPort)
}