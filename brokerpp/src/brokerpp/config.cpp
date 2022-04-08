#include <brokerpp/config.hpp>
#include <brokerpp/load_home_file.hpp>

namespace TunnelBore::Broker
{

Config loadConfig(bool inDev)
{
    const auto configString = loadHomeFile(inDev ? "configDev.json" : "config.json");
    // TODO: 
    return Config{};
}
void saveConfig(Config const& config, bool inDev)
{
    // TODO: 
}

}