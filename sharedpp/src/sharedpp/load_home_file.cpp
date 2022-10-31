#include <sharedpp/load_home_file.hpp>

#include <fstream>
#include <roar/filesystem/special_paths.hpp>
#include <sstream>

using namespace std::literals;

namespace TunnelBore
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
        writer.write(data.c_str(), static_cast<std::streamsize>(data.size()));
    }
    std::filesystem::path getHomePath()
    {
        return Roar::resolvePath("~/.tbore");
    }
    void setupHome()
    {
        const auto homePath = getHomePath();
        if (!std::filesystem::exists(homePath))
            std::filesystem::create_directories(homePath);
    }
} // namespace TunnelBore