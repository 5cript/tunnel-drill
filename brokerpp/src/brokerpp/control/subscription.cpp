#include <brokerpp/control/subscription.hpp>

namespace TunnelBore::Broker
{
    //#####################################################################################################################
    Subscription::Subscription(FunctionType cb)
        : m_callback{std::move(cb)}
    {}
    //#####################################################################################################################
}