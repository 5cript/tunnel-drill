#pragma once

#include <boost/asio/ip/tcp.hpp>

namespace TunnelBore::PublicServer
{
    /**
     *  
     */
    class Publisher
    {
    public:
        Publisher(boost::asio::ip::tcp::socket&& socket);
        ~Publisher();
        Publisher(Publisher const&) = delete;
        Publisher(Publisher&&);
        Publisher& operator=(Publisher const&) = delete;
        Publisher& operator=(Publisher&&);

        void close();

    private:
        struct Implementation;
        std::unique_ptr <Implementation> impl_;
    };
}