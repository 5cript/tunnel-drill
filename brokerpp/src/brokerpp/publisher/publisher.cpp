#include <brokerpp/publisher/publisher.hpp>
#include <brokerpp/publisher/service.hpp>
#include <brokerpp/json.hpp>
#include <brokerpp/resolve.hpp>
#include <attender/session/uuid_session_cookie_generator.hpp>

#include <spdlog/spdlog.h>

#include <string>
#include <mutex>

using namespace std::literals;

namespace TunnelBore::Broker
{
//#####################################################################################################################
    struct Publisher::Implementation
    {
        boost::asio::io_context* context;
        attender::uuid_generator uuidGenerator;
        std::string identity;
        std::recursive_mutex serviceGuard;
        std::unordered_map<std::string, std::shared_ptr<Service>> services;
        std::weak_ptr<ControlSession> controlSession;

        Implementation(boost::asio::io_context* context, std::string identity)
            : context{context}
            , uuidGenerator{}
            , identity{std::move(identity)}
            , services{}
            , controlSession{}
        {
        }
    };
//#####################################################################################################################
    Publisher::Publisher(boost::asio::io_context* context, std::string identity)
        : impl_{std::make_unique<Implementation>(context, std::move(identity))}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    Publisher::~Publisher()
    {
        spdlog::info("Publisher '{}' was destroyed", impl_->identity);
    }
//---------------------------------------------------------------------------------------------------------------------
    Publisher::Publisher(Publisher&&) = default;
//---------------------------------------------------------------------------------------------------------------------
    Publisher& Publisher::operator=(Publisher&&) = default;
//---------------------------------------------------------------------------------------------------------------------
    std::weak_ptr<ControlSession> Publisher::getCurrentControlSession()
    {
        return impl_->controlSession;
    }
//---------------------------------------------------------------------------------------------------------------------
    void Publisher::setCurrentControlSession(std::weak_ptr<ControlSession> controlSession)
    {
        auto session = controlSession.lock();
        if (!session)
            return;

        impl_->controlSession = controlSession;

        // Note to myself: dont capture session here, or it would be indefinitely kept alive.
        session->subscribe("Handshake", [weak = weak_from_this(), controlSession](json const& j, std::string const& ref) 
        {
            spdlog::info("Publisher handshake received.");

            auto shared = weak.lock();
            if (!shared)
            {
                spdlog::info("Publisher is not alive anymore. Discarding handshake.");
                return true;
            }
            auto session = controlSession.lock();
            if (!session)
            {
                spdlog::info("Control session is not alive anymore. Discarding handshake.");
                return true;
            }

            if (!j.contains("services"))
            {
                spdlog::error("Handshake should contain services.");
                return session->respondWithError(ref, "Services missing on handshake."), false;
            }

            try 
            {
                auto services = j["services"].get<std::vector<ServiceInfo>>();
                shared->addServices(services);
            } 
            catch(std::exception const& exc) 
            {
                spdlog::error("Exception during handshake parsing: {}", exc.what());
                return session->respondWithError(ref, "Exception during handshake parsing: "s + exc.what()), false;
            }
            return true;
        });
    }
//---------------------------------------------------------------------------------------------------------------------
    bool Publisher::addService(ServiceInfo serviceInfo)
    {
        auto returnResult = [this](bool result) 
        {          
            if (auto controlSession = impl_->controlSession.lock(); controlSession)  
                controlSession->writeJson(json{
                    {"type", "ServiceStartResult"},
                    {"result", result}
                });
            return result;
        };

        std::scoped_lock lock{impl_->serviceGuard};
        for (auto const& [serviceId, service] : impl_->services)
        {
            if (serviceInfo.publicPort == service->info().publicPort)
            {
                // TODO: This does not change service config changes. How to handle this?
                spdlog::info("Service for '{}' with public port '{}' already exists.", impl_->identity, serviceInfo.publicPort);
                return returnResult(true);
            }
        }

        const auto serviceId = impl_->uuidGenerator.generate_id();
        auto service = std::make_shared<Service>(
            *impl_->context, 
            serviceInfo, 
            resolveServer(*impl_->context, serviceInfo.publicPort, false),
            weak_from_this(),
            serviceId
        );
        auto result = service->start();
        if (!result)
        {
            spdlog::error("Could not start service for '{}' with public port '{}'", impl_->identity, serviceInfo.publicPort);
            return returnResult(false);
        }
        spdlog::info("Added service for '{}' with public port '{}'.", impl_->identity, serviceInfo.publicPort);
        impl_->services[serviceId] = std::move(service);
        return returnResult(true);
    }
//---------------------------------------------------------------------------------------------------------------------
    void Publisher::addServices(std::vector<ServiceInfo> const& services)
    {
        std::scoped_lock lock{impl_->serviceGuard};
        for (auto const& serviceInfo : services)
            addService(serviceInfo);
    }
//---------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<Service> Publisher::getService(std::string const& id)
    {
        std::scoped_lock lock{impl_->serviceGuard};
        auto iter = impl_->services.find(id);
        if (iter == impl_->services.end())
            return {};
        else
            return iter->second;
    }
//---------------------------------------------------------------------------------------------------------------------
    void Publisher::clearServices()
    {
        impl_->services.clear();
    }
//#####################################################################################################################
}