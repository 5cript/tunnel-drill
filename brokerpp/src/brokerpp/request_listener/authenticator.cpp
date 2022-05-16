#include <brokerpp/winsock_first.hpp>
#include <brokerpp/request_listener/authenticator.hpp>

#include <brokerpp/json.hpp>

#include <spdlog/spdlog.h>

#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/traits.h>

using namespace boost::beast::http;

namespace TunnelBore::Broker
{
    Authenticator::Authenticator(std::string privateJwt)
        : authMutex_{}
        , privateJwt_{std::move(privateJwt)}
        , userControl_{}
    {}
    void Authenticator::auth(Roar::Session& session, Roar::EmptyBodyRequest&& request)
    {
        std::lock_guard lock{authMutex_};
        auto basic = request.basicAuth();
        if (!basic)
        {
            return (void)session.send<empty_body>(request)->rejectAuthorization("Basic realm=tunnelBore").commit();
        }
        auto user = userControl_.getUser(basic->user, basic->password);
        if (!user)
        {
            spdlog::warn("User '{}' failed to authenticate.", basic->user);
            return (void)session.send<empty_body>(request)->rejectAuthorization("Basic realm=tunnelBore").commit();
        }
        using traits = jwt::traits::nlohmann_json;
        const auto token = jwt::create<traits>()
                               .set_issuer("tunnelBore")
                               .set_type("JWS")
                               .set_payload_claim("identity", user->identity())
                               .set_issued_at(std::chrono::system_clock::now())
                               .set_expires_at(std::chrono::system_clock::now() + std::chrono::days{1})
                               .sign(jwt::algorithm::rs256{"", privateJwt_, "", ""});
        spdlog::info("User '{}' authenticated, sending token.", user->identity());

        // FIXME: set cookie?
        return (void)session.send<string_body>(request)
            ->status(status::ok)
            .contentType("application/json")
            .body(token)
            .commit();
    }
}