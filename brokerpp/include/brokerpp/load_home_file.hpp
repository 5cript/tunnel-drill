#pragma once

#include <filesystem>

namespace TunnelBore::Broker
{
    std::string loadHomeFile(std::filesystem::path const& subpath);
}