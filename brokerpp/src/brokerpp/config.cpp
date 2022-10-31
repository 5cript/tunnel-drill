#include <brokerpp/config.hpp>
#include <sharedpp/load_home_file.hpp>

namespace TunnelBore::Broker
{
    namespace detail
    {
#ifdef NDEBUG
        constexpr static auto inDev = false;
#else
        constexpr static auto inDev = true;
#endif
    }

    Config loadConfig()
    {
        const auto configString = loadHomeFile(detail::inDev ? "broker/configDev.json" : "broker/config.json");
        return json::parse(configString).get<Config>();
    }
    void saveConfig(Config const& config)
    {
        saveHomeFile(detail::inDev ? "broker/configDev.json" : "broker/config.json", json{config}.dump());
    }

}