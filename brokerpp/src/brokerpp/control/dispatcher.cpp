#include <brokerpp/control/dispatcher.hpp>

#include <spdlog/spdlog.h>

namespace TunnelBore::Broker
{
    //#####################################################################################################################
    void Dispatcher::dispatch(json const& msg, std::string const& ref)
    {
        const std::string type = msg["type"];

        std::scoped_lock lock{m_subscriberGuard};
        auto [begin, end] = m_subscribers.equal_range(type);
        if (begin == end)
            spdlog::warn("No subscriber for {}.", type);

        while (begin != end)
        {
            if (auto held = begin->second.lock(); held)
            {
                if (!std::invoke(*held, msg, ref))
                    break;
                ++begin;
            }
            else
                begin = m_subscribers.erase(begin);
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<Subscription> Dispatcher::subscribe(
        std::string const& messageType,
        std::function<bool(Subscription::ParameterType const&, std::string const&)> const& callback)
    {
        std::scoped_lock lock{m_subscriberGuard};
        std::shared_ptr<Subscription> subscriber = std::make_shared<Subscription>(callback);
        m_subscribers.emplace(messageType, subscriber);
        return subscriber;
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<Subscription> Dispatcher::subscribe(
        std::string const& type,
        std::function<void(Subscription::ParameterType const&, std::string const&)> const& callback)
    {
        return subscribe(
            type,
            std::function<bool(Subscription::ParameterType const&, std::string const&)>{
                [callback](Subscription::ParameterType const& p, std::string const& ref) -> bool {
                    callback(p, ref);
                    return true;
                }});
    }
    //#####################################################################################################################
}