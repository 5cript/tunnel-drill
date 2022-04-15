#pragma once

#include <filesystem>
#include <string>

namespace TunnelBore::Broker
{
    std::string loadHomeFile(std::filesystem::path const& subpath);
    void saveHomeFile(std::filesystem::path const& subpath, std::string data);
    std::filesystem::path getHomePath();
}