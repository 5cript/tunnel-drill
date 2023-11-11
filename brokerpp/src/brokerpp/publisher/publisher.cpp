#include <brokerpp/winsock_first.hpp>
#include <brokerpp/publisher/publisher.hpp>
#include <brokerpp/publisher/service.hpp>
#include <sharedpp/json.hpp>
#include <roar/dns/resolve.hpp>
#include <sharedpp/uuid_generator.hpp>

#include <spdlog/spdlog.h>

#include <string>
#include <mutex>

using namespace std::literals;

namespace TunnelBore::Broker
{
    // #####################################################################################################################
    struct Publisher::Implementation
    {
        boost::asio::any_io_executor executor;
        uuid_generator uuidGenerator;
        std::string identity;
        std::recursive_mutex serviceGuard;
        std::unordered_map<std::string, std::shared_ptr<Service>> services;
        std::weak_ptr<ControlSession> controlSession;

        Implementation(boost::asio::any_io_executor executor, std::string identity)
            : executor{std::move(executor)}
            , uuidGenerator{}
            , identity{std::move(identity)}
            , services{}
            , controlSession{}
        {}
    };
    // #####################################################################################################################
    Publisher::Publisher(boost::asio::any_io_executor executor, std::string identity)
        : impl_{std::make_unique<Implementation>(executor, std::move(identity))}
    {}
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
        {
            spdlog::error("Control session is not alive anymore. Discarding handshake subscription.");
            return;
        }

        impl_->controlSession = controlSession;

        // Note to myself: dont capture session here, or it would be indefinitely kept alive.
        session->subscribe(
            "Handshake", [weak = weak_from_this(), controlSession = session->weak_from_this()](json const& j, std::string const& ref) {
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
                catch (std::exception const& exc)
                {
                    spdlog::error("Exception during handshake parsing: {}", exc.what());
                    return session->respondWithError(ref, "Exception during handshake parsing: "s + exc.what()), false;
                }
                return true;
            });

        session->subscribe(
            "Ping", [weak = weak_from_this(), controlSession = session->weak_from_this(), pingCounter = 0](json const&, std::string const& ref) mutable{
                if (pingCounter == 0)
                {
                    spdlog::info("Ping received first or a thousandth time.");
                }
                ++pingCounter;
                if (pingCounter % 1000 == 0)
                    pingCounter = 0;

                auto shared = weak.lock();
                if (!shared)
                {
                    spdlog::info("Publisher is not alive anymore. Discarding ping.");
                    return true;
                }
                auto session = controlSession.lock();
                if (!session)
                {
                    spdlog::info("Control session is not alive anymore. Discarding ping.");
                    return true;
                }

                return session->respond(ref, json{{"type", "Pong"}}), true;
            });
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Publisher::addService(ServiceInfo serviceInfo)
    {
        spdlog::info("Adding service for '{}' with public port '{}'.", impl_->identity, serviceInfo.publicPort);
        auto returnResult = [this](bool result) {
            if (auto controlSession = impl_->controlSession.lock(); controlSession)
                controlSession->writeJson(json{{"type", "ServiceStartResult"}, {"result", result}});
            return result;
        };

        std::scoped_lock lock{impl_->serviceGuard};
        std::vector<std::string> recreatedServices;
        for (auto const& [serviceId, service] : impl_->services)
        {
            if (serviceInfo.publicPort == service->info().publicPort)
            {
                // TODO: This does not change service config changes. How to handle this?
                spdlog::info(
                    "Service for '{}' with public port '{}' already existed, recreate.",
                    impl_->identity,
                    serviceInfo.publicPort);
                recreatedServices.push_back(serviceId);
                return returnResult(true);
            }
        }

        for (auto const& serviceId : recreatedServices)
            impl_->services.erase(serviceId);

        const auto serviceId = impl_->uuidGenerator.generate_id();
        auto service = std::make_shared<Service>(
            impl_->executor,
            serviceInfo,
            Roar::Dns::resolveSingle(
                impl_->executor, "::", serviceInfo.publicPort, false, boost::asio::ip::resolver_base::flags::passive),
            weak_from_this(),
            serviceId);
        auto result = service->start();
        if (!result)
        {
            spdlog::error(
                "Could not start service for '{}' with public port '{}'", impl_->identity, serviceInfo.publicPort);
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
    // #####################################################################################################################
}