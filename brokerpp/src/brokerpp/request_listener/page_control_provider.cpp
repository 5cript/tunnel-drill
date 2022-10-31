#include <brokerpp/winsock_first.hpp>
#include <brokerpp/request_listener/page_control_provider.hpp>
#include <brokerpp/control/control_session.hpp>
#include <brokerpp/publisher/publisher.hpp>
#include <brokerpp/publisher/publisher_token.hpp>

#include <roar/utility/base64.hpp>
#include <sharedpp/jwt.hpp>
#include <spdlog/spdlog.h>

using namespace boost::beast::http;
using namespace Roar;

namespace TunnelBore::Broker
{
    // #####################################################################################################################
    struct PageAndControlProvider::Implementation
    {
        boost::asio::any_io_executor executor;
        std::string publicJwt;
        std::unordered_map<std::string, std::shared_ptr<Publisher>> publishers;

        std::mutex controlSessionMutex;
        std::unordered_map<std::string, std::shared_ptr<ControlSession>> controlSessions;
        std::filesystem::path servedDirectory;

        Implementation(boost::asio::any_io_executor executor, std::string publicJwt, std::filesystem::path directory)
            : executor{std::move(executor)}
            , publicJwt{std::move(publicJwt)}
            , publishers{}
            , controlSessionMutex{}
            , controlSessions{}
            , servedDirectory{std::move(directory)}
        {}
    };
    // #####################################################################################################################
    PageAndControlProvider::PageAndControlProvider(
        boost::asio::any_io_executor executor,
        std::string publicJwt,
        std::filesystem::path directory)
        : impl_{std::make_unique<Implementation>(std::move(executor), std::move(publicJwt), std::move(directory))}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(PageAndControlProvider);
    //---------------------------------------------------------------------------------------------------------------------
    Roar::ServeDecision PageAndControlProvider::serve(
        Roar::Session& /*session*/,
        Roar::EmptyBodyRequest const& /*request*/,
        Roar::FileAndStatus const& fileAndStatus,
        Roar::ServeOptions<PageAndControlProvider>& /*options*/)
    {
        spdlog::info("Serving {}", fileAndStatus.file.string());
        return Roar::ServeDecision::Continue;
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::filesystem::path PageAndControlProvider::getServedDirectory() const
    {
        return impl_->servedDirectory;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void PageAndControlProvider::status(Session& session, EmptyBodyRequest&& req)
    {
        session.send<empty_body>(req)->status(status::no_content).commit();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void PageAndControlProvider::redirect1(Session& session, EmptyBodyRequest&& req)
    {
        session.send<empty_body>(req)->status(status::moved_permanently).setHeader(field::location, "/").commit();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void PageAndControlProvider::redirect2(Session& session, EmptyBodyRequest&& req)
    {
        session.send<empty_body>(req)->status(status::moved_permanently).setHeader(field::location, "/").commit();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void PageAndControlProvider::redirect3(Session& session, EmptyBodyRequest&& req)
    {
        session.send<empty_body>(req)->status(status::moved_permanently).setHeader(field::location, "/").commit();
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

        auto token = verifyPublisherToken(Roar::base64Decode(*tokenData), impl_->publicJwt);
        if (!token)
            return closeWithFailure("Token was rejected.");

        spdlog::info("Upgrading to WebSocket session");
        session.upgrade(req)
            .then([weak = weak_from_this(), identity = token->identity()](std::shared_ptr<WebsocketSession> ws) {
                // TODO:
                spdlog::info("Upgrade complete");
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
    // #####################################################################################################################
}