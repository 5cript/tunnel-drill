#include <brokerpp/winsock_first.hpp>
#include <brokerpp/request_listener/page_control_provider.hpp>
#include <brokerpp/control/control_session.hpp>
#include <brokerpp/publisher/publisher.hpp>
#include <brokerpp/publisher/publisher_token.hpp>

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

        std::mutex controlSessionMutex;
        std::unordered_map<std::string, std::shared_ptr<ControlSession>> controlSessions;

        Implementation(boost::asio::any_io_executor executor, std::string publicJwt)
            : executor{std::move(executor)}
            , publicJwt{std::move(publicJwt)}
            , publishers{}
            , controlSessionMutex{}
            , controlSessions{}
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
            spdlog::warn("Publisher control socket was rejected: '{}'", reason);
            session.send<string_body>(req)
                ->rejectAuthorization("Bearer realm=tunnelBore")
                .body(reason)
                .commit()
                .fail([](auto) {});
        };

        auto tokenData = req.bearerAuth();
        if (!tokenData)
            return closeWithFailure("Missing bearer auth.");

        auto token = verifyPublisherToken(*tokenData, impl_->publicJwt);
        if (!token)
            return closeWithFailure("Token was rejected.");

        // session.setup(ident->second.to_json().get<std::string>());

        session.upgrade(req)
            .then([weak = weak_from_this(), identity = token->identity()](std::shared_ptr<WebsocketSession> ws) {
                // TODO:
                auto self = weak.lock();
                if (!self)
                    return;

                auto cs = std::make_shared<ControlSession>(
                    identity,
                    weak,
                    std::move(ws),
                    [weak, identity]() {
                        auto self = weak.lock();
                        if (!self)
                            return;

                        std::scoped_lock lock{self->impl_->controlSessionMutex};
                        self->impl_->controlSessions.erase(identity);
                    },
                    self->impl_->publicJwt);
                cs->setup(identity);
                std::scoped_lock lock{self->impl_->controlSessionMutex};
                self->impl_->controlSessions[identity] = cs;
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