#pragma once

#include <string>
#include <fstream>
#include <memory>

namespace TunnelBore::Broker
{
    class TunnelSession;

    class PipeOperation : public std::enable_shared_from_this<PipeOperation>
    {
    public:
        PipeOperation(std::weak_ptr<TunnelSession> sideOriginal, std::weak_ptr<TunnelSession> sideOther);
        ~PipeOperation();
        PipeOperation(PipeOperation const&) = delete;
        PipeOperation(PipeOperation&&) = delete;
        PipeOperation& operator=(PipeOperation const&) = delete;
        PipeOperation& operator=(PipeOperation&&) = delete;

        void doPipe();

    private:
        void read();
        void write(std::size_t bytesTransferred, bool close);

    private:
        std::weak_ptr<TunnelSession> sideOriginal_;
        std::weak_ptr<TunnelSession> sideOther_;
        std::string buffer_;
        std::size_t totalTransfer_;
    };
}