#include <brokerpp/authority.hpp>
#include <sharedpp/json.hpp>

#include <spdlog/spdlog.h>
#include <sharedpp/jwt.hpp>

namespace TunnelBore::Broker
{
    // #####################################################################################################################
    Authority::Authority(std::string privateJwt)
        : authMutex_{}
        , privateJwt_{std::move(privateJwt)}
        , userControl_{}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    std::optional<std::string>
    Authority::authenticateThenSign(std::string const& name, std::string const& password, json const& claims)
    {
        std::scoped_lock lock{authMutex_};
        auto user = userControl_.getUser(name, password);
        if (!user)
        {
            spdlog::warn("User '{}' failed to authenticate.", name);
            return std::nullopt;
        }
        using traits = jwt::traits::nlohmann_json;
        auto token = jwt::create<traits>()
                         .set_issuer("tunnelBore")
                         .set_type("JWS")
                         .set_issued_at(std::chrono::system_clock::now())
                         .set_expires_at(std::chrono::system_clock::now() + std::chrono::days{365})
                         .set_payload_claim("identity", user->identity());
        for (auto const& claim : claims.items())
        {
            token.set_payload_claim(claim.key(), claim.value());
        }
        const auto signedToken = token.sign(jwt::algorithm::rs256{"", privateJwt_, "", ""});
        spdlog::info("User '{}' authenticated.", user->identity());
        return signedToken;
    }
    // #####################################################################################################################
}