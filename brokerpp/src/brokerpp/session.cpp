#include <brokerpp/session.hpp>

namespace TunnelBore::Broker
{
//#####################################################################################################################
    struct Session::Implementation
    {
        boost::asio::ip::tcp::socket socket_;

        Implementation(boost::asio::ip::tcp::socket&& socket)
            : socket_{std::move(socket)}
        {}
    };
//#####################################################################################################################
    Session::Session(boost::asio::ip::tcp::socket&& socket)
        : impl_{std::make_unique<Implementation>(std::move(socket))}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    void Session::close()
    {
        boost::system::error_code ignore;
        impl_->socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
    }
//---------------------------------------------------------------------------------------------------------------------
    Session::~Session()
    {
        close();
    }
//---------------------------------------------------------------------------------------------------------------------
    Session::Session(Session&&) = default;
//---------------------------------------------------------------------------------------------------------------------
    Session& Session::operator=(Session&&) = default;
//#####################################################################################################################
}