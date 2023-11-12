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
        PipeOperation(TunnelSession* sideOriginal, TunnelSession* sideOther)
            : sideOriginal_(sideOriginal)
            , sideOther_(sideOther)
            , state_(std::make_shared<State>([]() {
                return State{.buffer = std::string(4096, '\0'), .totalTransfer = 0};
            }()))
        {}
        ~PipeOperation()
        {
            spdlog::info("PipeOperation::~PipeOperation: total transfer: {}", state_->totalTransfer);
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
            sideOriginal_->socket().async_read_some(
                boost::asio::buffer(state_->buffer),
                [weakOperation = this->weak_from_this(),
                 state = this->state_](auto const& ec, std::size_t bytesTransferred) {
                    auto operation = weakOperation.lock();
                    if (!operation)
                    {
                        spdlog::error("Pipe operation died while piping (weakOperation::lock)");
                        return;
                    }

                    state->totalTransfer += bytesTransferred;

                    auto sideOriginal = operation->sideOriginal_;

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
        void write(std::size_t bytesTransferred, bool close, std::size_t cumulativeOffset = 0, int retries = 0)
        {
            sideOther_->resetTimer();

            if (retries > 10)
            {
                spdlog::error("Too many retries, killing pipe");

                if (sideOriginal_)
                    sideOriginal_->close();
                if (sideOther_)
                    sideOther_->close();
                return;
            }

            if (bytesTransferred > state_->buffer.size() || bytesTransferred == 0)
            {
                if (bytesTransferred > state_->buffer.size())
                    spdlog::error("bytesTransferred is too large, killing pipe: {}", bytesTransferred);

                if (sideOriginal_)
                    sideOriginal_->close();
                if (sideOther_)
                    sideOther_->close();
                return;
            }

            boost::asio::async_write(
                sideOther_->socket(),
                boost::asio::buffer(state_->buffer.data() + cumulativeOffset, bytesTransferred),
                [weakOperation = this->weak_from_this(),
                 state = this->state_,
                 close,
                 expectedWrittenAmount = bytesTransferred,
                 cumulativeOffset,
                 retries](auto const& ec, std::size_t bytesWritten) {
                    auto operation = weakOperation.lock();
                    if (!operation)
                    {
                        spdlog::error("Pipe operation died while piping (weakOperation::lock)");
                        return;
                    }

                    auto sideOriginal = operation->sideOriginal_;
                    auto sideOther = operation->sideOther_;

                    if (bytesWritten > expectedWrittenAmount)
                    {
                        spdlog::error("bytesWritten is too large, killing pipe: {}", bytesWritten);
                        if (sideOriginal)
                            sideOriginal->close();
                        if (sideOther)
                            sideOther->close();
                        return;
                    }

                    if (expectedWrittenAmount != bytesWritten)
                    {
                        spdlog::warn(
                            "Expected to write {} bytes, but only wrote {} bytes, retrying",
                            expectedWrittenAmount,
                            bytesWritten);
                        operation->write(
                            expectedWrittenAmount - bytesWritten, close, cumulativeOffset + bytesWritten, retries + 1);
                        return;
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
                });
        }

      private:
        TunnelSession* sideOriginal_;
        TunnelSession* sideOther_;
        struct State
        {
            std::string buffer;
            std::size_t totalTransfer;
        };
        std::shared_ptr<State> state_;
    };
}
//---------------------------------------------------------------------------------------------------------------------