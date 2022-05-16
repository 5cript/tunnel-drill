#include <brokerpp/winsock_first.hpp>
#include <brokerpp/request_listener/page_control_provider.hpp>
#include <brokerpp/control/control_session.hpp>
#include <brokerpp/publisher/publisher.hpp>

#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/traits.h>
#include <spdlog/spdlog.h>

using namespace boost::beast::http;
using namespace Roar;

namespace TunnelBore::Broker
{
    //#####################################################################################################################
    struct PageAndControlProvider::Implementation
    {
        boost::asio::any_io_executor executor;
        std::string publicJwt;
        std::unordered_map<std::string, std::shared_ptr<Publisher>> publishers;

        Implementation(boost::asio::any_io_executor executor, std::string publicJwt)
            : executor{std::move(executor)}
            , publicJwt{std::move(publicJwt)}
            , publishers{}
        {}
    };
    //#####################################################################################################################
    PageAndControlProvider::PageAndControlProvider(boost::asio::any_io_executor executor, std::string publicJwt)
        : impl_{std::make_unique<Implementation>(std::move(executor), std::move(publicJwt))}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(PageAndControlProvider);
    //---------------------------------------------------------------------------------------------------------------------
    void PageAndControlProvider::index(Session& session, EmptyBodyRequest&& req)
    {
        session.send<string_body>(req)->status(status::ok).contentType("text/plain").body("Hi").commit();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void PageAndControlProvider::publisher(Session& session, EmptyBodyRequest&& req)
    {
        auto closeWithFailure = [&session, &req](std::string_view reason) {
            session.send<string_body>(req)
                ->rejectAuthorization("Bearer realm=tunnelBore")
                .body(reason)
                .commit()
                .fail([](auto) {});
        };

        auto token = req.bearerAuth();
        if (!token)
            return closeWithFailure("Bearer realm=tunnelBore");

        const auto decoded = jwt::decode<jwt::traits::nlohmann_json>(*token);

        const auto verifier = jwt::verify<jwt::traits::nlohmann_json>()
                                  .allow_algorithm(jwt::algorithm::rs256{impl_->publicJwt, "", "", ""})
                                  .with_issuer("tunnelBore");
        std::error_code ec;
        verifier.verify(decoded, ec);
        if (ec)
            return closeWithFailure("Token was rejected.");

        const auto claims = decoded.get_payload_claims();
        auto ident = claims.find("identity");
        if (ident == std::end(claims))
            return closeWithFailure("Token lacks identity claim.");

        // session.setup(ident->second.to_json().get<std::string>());

        session.upgrade(req)
            .then(
                [weak = weak_from_this(), identity = ident->second.as_string()](std::shared_ptr<WebsocketSession> ws) {
                    // TODO:
                    auto cs = std::make_shared<ControlSession>(identity, weak, std::move(ws));
                })
            .fail([](Error const& e) {
                spdlog::error("Websocket upgrade failed: '{}'.", e.toString());
            });
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<Publisher> PageAndControlProvider::obtainPublisher(std::string const& identity)
    {

        auto pubIter = impl_->publishers.find(identity);
        if (pubIter == impl_->publishers.end())
        {
            auto publisher = std::make_shared<Publisher>(impl_->executor, identity);
            impl_->publishers[identity] = publisher;
            return publisher;
        }
        return pubIter->second;
    }
    //#####################################################################################################################
}