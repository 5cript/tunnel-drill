#pragma once

#include <sharedpp/json.hpp>
#include <publisherpp/service_info.hpp>

namespace TunnelBore::Publisher
{
    struct Config
    {
        std::string identity;
        std::string passHashed;
        std::string host;
        int port;
        std::string authorityHost;
        int authorityPort;
        std::vector<ServiceInfo> services;
        bool ssl = true;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Config, identity, passHashed, host, port, authorityHost, authorityPort, services, ssl)

    Config loadConfig();
    void saveConfig(Config const& config);
}