#include <brokerpp/winsock_first.hpp>
#include <brokerpp/request_listener/authenticator.hpp>
#include <sharedpp/json.hpp>

#include <roar/mechanics/cookie.hpp>

#include <spdlog/spdlog.h>
#include <sharedpp/jwt.hpp>

using namespace boost::beast::http;

namespace TunnelBore::Broker
{
    Authenticator::Authenticator(std::shared_ptr<Authority> authority)
        : authority_{std::move(authority)}
    {}
    void Authenticator::auth(Roar::Session& session, Roar::EmptyBodyRequest&& request)
    {
        auto basic = request.basicAuth();
        if (!basic)
        {
            spdlog::warn("User '{}' failed to provide basic auth.", basic->user);
            return (void)session.send<empty_body>(request)->rejectAuthorization("Basic realm=tunnelBore").commit();
        }
        const auto token = authority_->authenticateThenSign(basic->user, basic->password);
        if (!token)
        {
            spdlog::warn("User '{}' failed to authenticate.", basic->user);
            return (void)session.template send<empty_body>(request)
                ->rejectAuthorization("Basic realm=tunnelBore")
                .commit();
        }

        return (void)session.send<string_body>(request)
            ->status(status::ok)
            .contentType("application/json")
            .body(*token)
            .commit();
    }
    void Authenticator::signJson(Roar::Session& session, Roar::EmptyBodyRequest&& request)
    {
        auto basic = request.basicAuth();
        if (!basic)
        {
            spdlog::warn("User '{}' failed to provide basic auth.", basic->user);
            return (void)session.send<empty_body>(request)
                ->rejectAuthorization("Basic realm=tunnelBore")
                .commit()
                .fail([](auto err) {
                    spdlog::error("Failed to send auth rejection response: {}", err.toString());
                });
        }

        session.read<string_body>(request)
            ->commit()
            .then([basic = *basic, weak = weak_from_this()](auto& session, auto const& req) {
                auto self = weak.lock();
                if (!self)
                {
                    spdlog::error("Authenticator was destroyed before request was completed.");
                    return;
                }

                json claims;
                try
                {
                    claims = json::parse(req.body());
                }
                catch (std::exception const& exc)
                {
                    spdlog::warn("Could not parse claims: '{}'", exc.what());
                    return (void)session.template send<empty_body>(req)
                        ->rejectAuthorization("Basic realm=tunnelBore")
                        .commit()
                        .fail([](auto err) {
                            spdlog::error("Failed to send auth rejection response: {}", err.toString());
                        });
                }
                const auto token = self->authority_->authenticateThenSign(basic.user, basic.password, claims);
                if (!token)
                {
                    spdlog::warn("User '{}' failed to authenticate.", basic.user);
                    return (void)session.template send<empty_body>(req)
                        ->rejectAuthorization("Basic realm=tunnelBore")
                        .commit()
                        .fail([](auto err) {
                            spdlog::error("Failed to send auth rejection response: {}", err.toString());
                        });
                }
                session.template send<string_body>(req)
                    ->status(status::ok)
                    .contentType("application/json")
                    .body(json{
                        {"token", *token},
                    })
                    .commit()
                    .fail([](auto err) {
                        spdlog::error("Failed to send back signed token: {}", err.toString());
                    });
            })
            .fail([](auto err) {
                spdlog::error("Failed to read payload body: {}", err.toString());
            });
    }
}