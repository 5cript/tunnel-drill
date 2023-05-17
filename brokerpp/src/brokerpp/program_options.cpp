#include <brokerpp/program_options.hpp>

#include <cxxopts.hpp>

namespace TunnelBore::Broker
{
    ProgramOptions parseProgramOptions(int argc, char** argv)
    {
        cxxopts::Options options("connection broker", "Establishes connections between hidden servers and clients.");

        options.add_options()("served-directory", "Directory to serve.", cxxopts::value<std::string>()->default_value("./httpdocs"));

        const auto result = options.parse(argc, argv);

        return ProgramOptions{.servedDirectory = result["served-directory"].as<std::string>()};
    }
}