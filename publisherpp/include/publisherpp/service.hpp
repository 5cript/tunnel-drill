#pragma once

#include <publisherpp/service_session.hpp>

#include <sharedpp/json.hpp>
#include <boost/asio/any_io_executor.hpp>

#include <unordered_map>

namespace TunnelBore::Publisher
{
    class Service : public std::enable_shared_from_this<Service>
    {
        struct ServiceSessionPair
        {
            std::shared_ptr<ServiceSession> inward;
            std::shared_ptr<ServiceSession> outward;
            std::shared_ptr<PipeOperation<ServiceSession>> inwardPipe;
            std::shared_ptr<PipeOperation<ServiceSession>> outwardPipe;
        };

      public:
        Service(
            boost::asio::any_io_executor executor,
            std::optional<std::string> name,
            int publicPort,
            std::string hiddenHost,
            int hiddenPort);

        void createSession(std::string const& brokerHost, std::string const& token, std::string const& tunnelId);

        std::string name() const;
        int publicPort() const;
        std::string const& hiddenHost() const;
        int hiddenPort() const;

        friend void to_json(nlohmann::json& j, const Service& v)
        {
            j = nlohmann::json{
                {"name", v.name_},
                {"publicPort", v.publicPort_},
                {"hiddenHost", v.hiddenHost_},
                {"hiddenPort", v.hiddenPort_}};
        }

        friend void from_json(const nlohmann::json& j, Service& v)
        {
            j.at("name").get_to(v.name_);
            j.at("publicPort").get_to(v.publicPort_);
            j.at("hiddenHost").get_to(v.hiddenHost_);
            j.at("hiddenPort").get_to(v.hiddenPort_);
        }

      private:
        boost::asio::any_io_executor executor_;
        std::optional<std::string> name_;
        int publicPort_;
        std::string hiddenHost_;
        int hiddenPort_;
        std::recursive_mutex sessionGuard_;
        std::unordered_map<std::string, ServiceSessionPair> sessions_;
    };
}