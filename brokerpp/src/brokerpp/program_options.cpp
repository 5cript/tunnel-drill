#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <brokerpp/program_options.hpp>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

namespace TunnelBore::Broker
{
    ProgramOptions parseProgramOptions(int argc, char** argv)
    {
        po::options_description desc("Program options");
        desc.add_options()(
            "served-directory,s", po::value<std::string>()->default_value("./httpdocs"), "Directory to serve.");
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        return ProgramOptions{.servedDirectory = vm["served-directory"].as<std::string>()};
    }
}