#include <brokerpp/publisher/publisher_token.hpp>

#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/traits.h>

namespace TunnelBore::Broker
{
    //#####################################################################################################################
    PublisherToken::PublisherToken(std::string identity, json otherClaims)
        : identity_{std::move(identity)}
        , otherClaims_{std::move(otherClaims)}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    std::string PublisherToken::identity() const
    {
        return identity_;
    }
    //---------------------------------------------------------------------------------------------------------------------
    json PublisherToken::claims() const
    {
        return otherClaims_;
    }
    //#####################################################################################################################
    std::optional<PublisherToken> verifyPublisherToken(std::string const& tokenData, std::string const& publicJwtKey)
    {
        const auto decoded = jwt::decode<jwt::traits::nlohmann_json>(tokenData);

        const auto verifier = jwt::verify<jwt::traits::nlohmann_json>()
                                  .allow_algorithm(jwt::algorithm::rs256{publicJwtKey, "", "", ""})
                                  .with_issuer("tunnelBore");
        std::error_code ec;
        verifier.verify(decoded, ec);
        if (ec)
            return std::nullopt;

        const auto claims = decoded.get_payload_claims();
        auto ident = claims.find("identity");
        if (ident == std::end(claims))
            return std::nullopt;
        json jsonClaims;
        for (auto const& claim : claims)
        {
            jsonClaims[claim.first] = claim.second.to_json();
        }
        return PublisherToken{ident->second.as_string(), jsonClaims};
    }
    //#####################################################################################################################
}