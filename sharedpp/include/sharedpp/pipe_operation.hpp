#pragma once

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

#include <string>
#include <fstream>
#include <memory>

namespace TunnelBore
{
    template <typename TunnelSession>
    class PipeOperation : public std::enable_shared_from_this<PipeOperation<TunnelSession>>
    {
      public:
        PipeOperation(std::weak_ptr<TunnelSession> sideOriginal, std::weak_ptr<TunnelSession> sideOther)
            : sideOriginal_(sideOriginal)
            , sideOther_(sideOther)
            , buffer_(4096, '\0')
        {}
        ~PipeOperation()
        {
        }
        PipeOperation(PipeOperation const&) = delete;
        PipeOperation(PipeOperation&&) = delete;
        PipeOperation& operator=(PipeOperation const&) = delete;
        PipeOperation& operator=(PipeOperation&&) = delete;

        void doPipe()
        {
            read();
        }

      private:
        void read()
        {
            auto sideOriginal = sideOriginal_.lock();
            if (!sideOriginal)
                return;

            sideOriginal->socket().async_read_some(
                boost::asio::buffer(buffer_),
                [operation = this->shared_from_this()](auto const& ec, std::size_t bytesTransferred) {
                    auto sideOriginal = operation->sideOriginal_.lock();

                    if (!sideOriginal)
                        return;
                    else
                    {
                        sideOriginal->resetTimer();
                        if (ec && ec != boost::asio::error::eof)
                            spdlog::warn(
                                "Error in pipeTo(1) in tunnel '{}': '{}'", sideOriginal->remoteAddress(), ec.message());
                    }
                    operation->write(bytesTransferred, ec.operator bool());
                });
        }
        void write(std::size_t bytesTransferred, bool close)
        {
            auto sideOther = sideOther_.lock();
            if (!sideOther)
                return;
            sideOther->resetTimer();

            if (bytesTransferred > buffer_.size() || bytesTransferred == 0)
            {
                if (bytesTransferred > buffer_.size())
                    spdlog::error("bytesTransferred is too large, killing pipe: {}", bytesTransferred);
                auto sideOriginal = sideOriginal_.lock();

                if (sideOriginal)
                    sideOriginal->close();
                if (sideOther)
                    sideOther->close();
                return;
            }

            auto onWriteDone = std::make_shared<std::function<void(boost::system::error_code, std::size_t)>>();
            *onWriteDone = [operation = this->shared_from_this(), close, onWriteDone, expectedWrittenAmount = bytesTransferred] mutable
            (auto const& ec, std::size_t bytesWritten)
            {
                Roar::ScopeExit onExit([onWriteDone]() { onWriteDone.reset(); });

                auto sideOriginal = operation->sideOriginal_.lock();
                auto sideOther = operation->sideOther_.lock();

                if (expectedWrittenAmount != bytesWritten)
                {
                    spdlog::warn("bytesWritten is not equal to bytesTransferred, killing pipe: expected {} != written {}", expectedWrittenAmount, bytesWritten);
                    onExit.disarm();
                    boost::asio::async_write(
                        sideOther->socket(),
                        boost::asio::buffer(buffer_.data() + bytesWritten, expectedWrittenAmount - bytesWritten),
                        *onWriteDone
                    );

                    // if (sideOriginal)
                    //     sideOriginal->close();
                    // if (sideOther)
                    //     sideOther->close();
                    // return;
                }

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
                        spdlog::warn(
                            "Error in pipeTo(2) in tunnel '{}': '{}'", sideOther->remoteAddress(), ec.message());

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
            };

            boost::asio::async_write(
                sideOther->socket(),
                boost::asio::buffer(buffer_, bytesTransferred),
                *onWriteDone
            );
        }

      private:
        std::weak_ptr<TunnelSession> sideOriginal_;
        std::weak_ptr<TunnelSession> sideOther_;
        std::string buffer_;
        std::size_t totalTransfer_;
    };
}
//---------------------------------------------------------------------------------------------------------------------