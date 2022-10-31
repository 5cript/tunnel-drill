#pragma once

#include "subscription.hpp"

#include <sharedpp/json.hpp>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <optional>

namespace TunnelBore::Broker
{
    class Dispatcher final
    {
      public:
        [[nodiscard]] std::shared_ptr<Subscription> subscribe(
            std::string const& type,
            std::function<bool(Subscription::ParameterType const&, std::string const&)> const& callback);

        [[nodiscard]] std::shared_ptr<Subscription> subscribe(
            std::string const& type,
            std::function<void(Subscription::ParameterType const&, std::string const&)> const& callback);

        void dispatch(json const& msg, std::string const& ref);

      private:
        std::unordered_multimap<std::string, std::weak_ptr<Subscription>> m_subscribers;
        std::mutex m_subscriberGuard;
    };
}