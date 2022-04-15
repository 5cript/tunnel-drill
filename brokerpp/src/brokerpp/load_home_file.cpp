#include <brokerpp/load_home_file.hpp>

#include <special-paths/special_paths.hpp>
#include <fstream>
#include <sstream>

using namespace std::literals;

namespace TunnelBore::Broker
{
    std::string loadHomeFile(std::filesystem::path const& subpath)
    {
        const auto path = getHomePath() / subpath;
        std::ifstream reader{path, std::ios_base::binary};
        if (!reader.good())
            throw std::runtime_error("Cannot load home file "s + path.string());
        std::stringstream sstr;
        sstr << reader.rdbuf();
        return sstr.str();
    }
    void saveHomeFile(std::filesystem::path const& subpath, std::string data)
    {
        const auto path = getHomePath() / subpath;
        std::ofstream writer{path, std::ios_base::binary};
        if (!writer.good())
            throw std::runtime_error("Cannot open home file for writing "s + path.string());
        writer.write(data.c_str(), static_cast <std::streamsize>(data.size()));
    }
    std::filesystem::path getHomePath()
    {
        return SpecialPaths::getHome() / ".tbore";
    }
}