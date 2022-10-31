#pragma once

#include <sharedpp/json.hpp>

namespace TunnelBore::Broker
{
    struct ServerConfig
    {
        std::string iface;
        unsigned short port;
    };
    struct Config
    {
        ServerConfig bind;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ServerConfig, iface, port)
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Config, bind)

    Config loadConfig();
    void saveConfig(Config const& config);

}