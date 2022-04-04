#include <public-server/publisher.hpp>

namespace TunnelBore::PublicServer
{
//#####################################################################################################################
    struct Publisher::Implementation
    {
        boost::asio::ip::tcp::socket socket_;

        Implementation(boost::asio::ip::tcp::socket&& socket)
            : socket_{std::move(socket)}
        {}
    };
//#####################################################################################################################
    Publisher::Publisher(boost::asio::ip::tcp::socket&& socket)
        : impl_{std::make_unique<Implementation>(std::move(socket))}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    void Publisher::close()
    {
        boost::system::error_code ignore;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
    }
//---------------------------------------------------------------------------------------------------------------------
    Publisher::~Publisher()
    {
        close();
    }
//---------------------------------------------------------------------------------------------------------------------
    Publisher::Publisher(Publisher&&) = default;
//---------------------------------------------------------------------------------------------------------------------
    Publisher& Publisher::operator=(Publisher&&) = default;
//#####################################################################################################################
}