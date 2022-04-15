#pragma once

#include <brokerpp/json.hpp>

namespace TunnelBore::Broker
{

    struct Config
    {
        unsigned short controlPort;
        // Can in future hopefully be controlPort or removed due to keycloak, auth0, ...
        unsigned short authorityPort;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Config, controlPort, authorityPort)

    Config loadConfig();
    void saveConfig(Config const& config);

}