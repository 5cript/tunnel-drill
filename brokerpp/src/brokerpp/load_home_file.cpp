#include <brokerpp/load_home_file.hpp>

#include <special-paths/special_paths.hpp>
#include <fstream>

using namespace std::literals;

namespace TunnelBore::Broker
{
    std::string loadHomeFile(std::filesystem::path const& subpath)
    {
        const auto home = SpecialPaths::getHome() / ".tbore" / subpath;
        std::ifstream reader{home, std::ios_base::binary};
        if (!reader.good()) {
            throw std::runtime_error("Cannot load home file "s + home.string());
        }
        std::stringstream sstr;
        sstr << reader.rdbuf();
        return sstr.str();
    }
}