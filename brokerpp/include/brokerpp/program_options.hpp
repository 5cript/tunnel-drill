#pragma once

#include <string>

namespace TunnelBore::Broker
{
    struct ProgramOptions
    {
        std::string servedDirectory;
    };

    ProgramOptions parseProgramOptions(int argc, char** argv);
}