#include <brokerpp/publisher/pipe_operation.hpp>
#include <brokerpp/publisher/tunnel_session.hpp>

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

#include <iostream>

using namespace std::string_literals;

namespace TunnelBore::Broker 
{
    //#####################################################################################################################
    PipeOperation::PipeOperation(std::weak_ptr<TunnelSession> sideOriginal, std::weak_ptr<TunnelSession> sideOther)
        : sideOriginal_(sideOriginal)
        , sideOther_(sideOther)
        , buffer_(65536, '\0')
        , totalTransfer_{0}
    {
    }
    //---------------------------------------------------------------------------------------------------------------------
    PipeOperation::~PipeOperation()
    {
        spdlog::debug("PipeOperation::~PipeOperation()");
        spdlog::info("Pipe operation total transfer: {}.", operation->totalTransfer_);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void PipeOperation::doPipe()
    {
        read();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void PipeOperation::read()
    {
        auto sideOriginal = sideOriginal_.lock();
        if (!sideOriginal)
            return;

        sideOriginal->socket().async_read_some(
            boost::asio::buffer(buffer_),
            [operation = shared_from_this()](auto const& ec, std::size_t bytesTransferred) {
                auto sideOriginal = operation->sideOriginal_.lock();
                operation->totalTransfer_ += bytesTransferred;

                if (!sideOriginal)
                    return;
                else
                {
                    sideOriginal->resetTimer();           
                    if (ec && ec != boost::asio::error::eof)
                        spdlog::warn("Error in pipeTo(1) in tunnel '{}': '{}'", sideOriginal->remoteAddress(), ec.message());
                }
                operation->write(bytesTransferred, ec.operator bool());
            });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void PipeOperation::write(std::size_t bytesTransferred, bool close)
    {
        auto sideOther = sideOther_.lock();
        if (!sideOther)
            return;    
        sideOther->resetTimer();
        boost::asio::async_write(
            sideOther->socket(),
            boost::asio::buffer(buffer_, bytesTransferred),
            [operation = shared_from_this(), close](auto const& ec, std::size_t) {
                auto sideOriginal = operation->sideOriginal_.lock();
                auto sideOther = operation->sideOther_.lock();

                if (!sideOriginal)
                {
                    spdlog::error("Tunnel session died while piping (sideOriginal::write)");
                }
                if (!sideOther)
                {
                    spdlog::error("Tunnel session died while piping (sideOther::write)");
                }

                if (ec || close)
                {
                    if (ec && sideOther)
                        spdlog::warn("Error in pipeTo(2) in tunnel '{}': '{}'", sideOther->remoteAddress(), ec.message());

                    if (sideOriginal)
                        sideOriginal->close();
                    if (sideOther)
                        sideOther->close();
                    return;
                }

                if (sideOriginal)
                    sideOriginal->resetTimer();
                if (sideOther)
                    sideOther->resetTimer();

                operation->read();
            });
    }
    //#####################################################################################################################
}