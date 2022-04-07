#pragma once

#include <brokerpp/json.hpp>

#include <functional>

namespace TunnelBore::Broker
{
    class Subscription
    {
      public:
        using ParameterType = json;
        using FunctionType = std::function<bool(ParameterType const&, std::string const& ref)>;

        explicit Subscription(FunctionType cb);
        bool operator()(ParameterType const& param, std::string const& ref)
        {
            return m_callback(param, ref);
        }

      private:
        FunctionType m_callback;
    };
}