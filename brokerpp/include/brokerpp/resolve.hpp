#include <boost/asio/ip/tcp.hpp>

namespace TunnelBore::Broker
{
    inline boost::asio::ip::tcp::resolver::endpoint_type resolveServer(boost::asio::io_context& context, unsigned short port, bool preferIpv4)
    {
        boost::asio::ip::tcp::resolver resolver{context};
        typename boost::asio::ip::tcp::resolver::iterator end, start = resolver.resolve("::", std::to_string(port));

        if (end == start)
            throw std::runtime_error("Cannot resolve hostname to bind tcp servers.");

        std::vector<typename boost::asio::ip::tcp::resolver::endpoint_type> endpoints{start, end};
        std::partition(std::begin(endpoints), std::end(endpoints), [preferIpv4](auto const& lhs) {
            return lhs.address().is_v4() == preferIpv4;
        });
        return endpoints[0];
    }
}