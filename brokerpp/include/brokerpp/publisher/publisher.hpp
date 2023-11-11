#pragma once

#include <brokerpp/control/dispatcher.hpp>
#include <brokerpp/publisher/service_info.hpp>
#include <brokerpp/control/control_session.hpp>

#include <memory>

namespace TunnelBore::Broker
{
    class Service;

    class Publisher : public std::enable_shared_from_this<Publisher>
    {
      public:
        Publisher(boost::asio::any_io_executor executor, std::string identity);
        ~Publisher();
        Publisher(Publisher const&) = delete;
        Publisher(Publisher&&);
        Publisher& operator=(Publisher const&) = delete;
        Publisher& operator=(Publisher&&);

        void setCurrentControlSession(std::weak_ptr<ControlSession> controlSession);
        std::weak_ptr<ControlSession> getCurrentControlSession();
        void detachControlSession(bool eraseServices);

        Service* getService(std::string const& id);
        std::vector<std::string> getServiceIds() const;
        std::size_t removeService(std::string const& id);

        bool addService(ServiceInfo serviceInfo);

      private:
        void addServices(std::vector<ServiceInfo> const& services);
        void clearServices();

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}