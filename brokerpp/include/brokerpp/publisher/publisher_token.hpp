#pragma once

#include <brokerpp/json.hpp>

#include <string>
#include <optional>

namespace TunnelBore::Broker
{
    class PublisherToken
    {
      public:
        PublisherToken(std::string identity, json otherClaims);

        std::string identity() const;
        json claims() const;

      private:
        std::string identity_;
        json otherClaims_;
    };

    std::optional<PublisherToken> verifyPublisherToken(std::string const& tokenData, std::string const& publicJwtKey);
}