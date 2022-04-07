#pragma once

#include <brokerpp/control/dispatcher.hpp>
#include <brokerpp/publisher/service_info.hpp>
#include <brokerpp/control/control_session.hpp>

#include <memory>

namespace TunnelBore::Broker
{
    class Publisher : std::enable_shared_from_this <Publisher>
    {
    public:
        Publisher(boost::asio::io_context* context);
        void subscribe(std::weak_ptr<ControlSession> controlSession);

    private:
        void setServices(std::vector<ServiceInfo> const& services, ControlSession* controlSession = nullptr);
        void clearServices();

    private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}