#pragma once

namespace TunnelBore::Broker
{

struct Config
{
    unsigned short port;
};

Config loadConfig(bool inDev);
void saveConfig(Config const& config, bool inDev);

}