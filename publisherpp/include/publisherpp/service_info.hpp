#pragma once

#include <sharedpp/json.hpp>

#include <string>
#include <optional>

namespace TunnelBore::Publisher
{
    struct ServiceInfo
    {
        std::optional<std::string> name;
        std::string socketType;
        int hiddenPort;
        int publicPort;
        std::optional<std::string> hiddenHost;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ServiceInfo, name, socketType, hiddenPort, publicPort, hiddenHost)
}