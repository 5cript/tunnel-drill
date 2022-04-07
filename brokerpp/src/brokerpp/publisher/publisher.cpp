#include <brokerpp/publisher/publisher.hpp>
#include <brokerpp/publisher/service.hpp>
#include <brokerpp/json.hpp>

#include <string>

// FIXME: move out.
constexpr bool preferIpv4 = false;

using namespace std::literals;

namespace TunnelBore::Broker
{
//#####################################################################################################################
    struct Publisher::Implementation
    {
        boost::asio::io_context* context;
        std::string identity;
        std::vector <std::unique_ptr<Service>> services;

        Implementation(boost::asio::io_context* context)
            : context{context}
            , identity{}
            , services{}
        {
        }
    };
//#####################################################################################################################
    Publisher::Publisher(boost::asio::io_context* context)
        : impl_{std::make_unique<Implementation>(context)}
    {
    }
//---------------------------------------------------------------------------------------------------------------------
    void Publisher::subscribe(std::weak_ptr<ControlSession> controlSession)
    {
        auto session = controlSession.lock();
        if (!session)
            return;

        session->subscribe("Handshake", [weak = weak_from_this(), controlSession](json const& j, std::string const& ref) {
            auto shared = weak.lock();
            if (!shared)
                return true;
            auto session = controlSession.lock();
            if (!session)
                return true;

            if (!j.contains("identity"))
                return session->respondWithError(ref, "Identity missing on handshake."), false;
            if (!j.contains("services"))
                return session->respondWithError(ref, "Services missing on handshake."), false;

            shared->impl_->identity = j["identity"].get<std::string>();

            std::vector <ServiceInfo> services;
            try 
            {
                const auto jsonServices = j["services"];
                for (json::const_iterator it = jsonServices.begin(); it != jsonServices.end(); ++it)
                {
                    ServiceInfo info;
                    if (it->contains("name"))
                        info.name = (*it)["name"].get<std::string>();

                    info.publicPort = (*it)["publicPort"].get<unsigned short>();
                    info.hiddenPort = (*it)["hiddenPort"].get<unsigned short>();
                    services.push_back(std::move(info));
                }

                shared->setServices(services, &*session);
            } 
            catch(std::exception const& exc) 
            {
                return session->respondWithError(ref, "Exception during handshake parsing: "s + exc.what()), false;
            }
            return true;
        });
    }
//---------------------------------------------------------------------------------------------------------------------
    void Publisher::setServices(std::vector<ServiceInfo> const& services, ControlSession* controlSession)
    {
        clearServices();

        auto resolve = [this](unsigned short port) {        
            boost::asio::ip::tcp::resolver resolver{*impl_->context};
            typename boost::asio::ip::tcp::resolver::iterator end, start = resolver.resolve("::", std::to_string(port));

            if (end == start)
                throw std::runtime_error("Cannot resolve hostname to bind tcp servers.");

            std::vector<typename boost::asio::ip::tcp::resolver::endpoint_type> endpoints{start, end};
            std::partition(std::begin(endpoints), std::end(endpoints), [](auto const& lhs) {
                return lhs.address().is_v4() == preferIpv4;
            });
            return endpoints[0];
        };

        for (auto const& serviceInfo : services)
        {
            auto service = std::make_unique<Service>(*impl_->context, serviceInfo, resolve(serviceInfo.publicPort));
            impl_->services.push_back(std::move(service));
        }
        for (auto const& service : impl_->services)
        {
            auto result = service->start();
            if (!result)
            {
                clearServices();
                // FIXME: use result to improve reporting:
                if (controlSession)
                    controlSession->respondWithError("", "Could not start service");
                return;
            }
        }
    }
//---------------------------------------------------------------------------------------------------------------------
    void Publisher::clearServices()
    {
        impl_->services.clear();
    }
//#####################################################################################################################
}