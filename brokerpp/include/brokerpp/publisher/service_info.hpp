#pragma once

#include <optional>
#include <string>

namespace TunnelBore::Broker
{

struct ServiceInfo
{
    std::optional <std::string> name;
    unsigned short publicPort;
    unsigned short hiddenPort;
};

}