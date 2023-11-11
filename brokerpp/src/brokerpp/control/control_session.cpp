#include <brokerpp/publisher/publisher_token.hpp>
#include <brokerpp/winsock_first.hpp>
#include <brokerpp/control/control_session.hpp>
#include <brokerpp/control/dispatcher.hpp>
#include <brokerpp/request_listener/page_control_provider.hpp>
#include <brokerpp/publisher/service.hpp>
#include <brokerpp/control/stream_parser.hpp>
#include <brokerpp/publisher/publisher.hpp>

#include <roar/utility/scope_exit.hpp>

#include <spdlog/spdlog.h>

#include <string>
#include <deque>
#include <mutex>

namespace TunnelBore::Broker
{
    // #####################################################################################################################
    namespace
    {
        struct WriteOperation
        {
            std::string payload;
        };
    }
    // #####################################################################################################################
    struct ControlSession::Implementation
    {
        std::string sessionId;
        std::weak_ptr<PageAndControlProvider> page_and_control;
        std::shared_ptr<Roar::WebsocketSession> ws;
        std::string identity;
        bool authenticated;
        StreamParser textParser;
        Dispatcher dispatcher;
        boost::asio::ip::tcp::endpoint remoteEndpoint;
        std::function<void()> endSelf;
        std::string publicJwtKey;

        std::vector<std::shared_ptr<Subscription>> subscriptions;

        std::recursive_mutex writeGuard;
        std::deque<WriteOperation> pendingMessages;
        bool writeInProgress;

        Implementation(
            std::string sessionId,
            std::weak_ptr<PageAndControlProvider> PageAndControlProvider,
            std::shared_ptr<Roar::WebsocketSession> ws,
            std::function<void()> endSelf,
            std::string publicJwtKey);
    };
    //---------------------------------------------------------------------------------------------------------------------
    ControlSession::Implementation::Implementation(
        std::string sessionId,
        std::weak_ptr<PageAndControlProvider> PageAndControlProvider,
        std::shared_ptr<Roar::WebsocketSession> ws,
        std::function<void()> endSelf,
        std::string publicJwtKey)
        : sessionId{std::move(sessionId)}
        , page_and_control{std::move(PageAndControlProvider)}
        , ws{std::move(ws)}
        , identity{}
        , authenticated{false}
        , textParser{}
        , dispatcher{}
        , remoteEndpoint{}
        , endSelf{std::move(endSelf)}
        , publicJwtKey{std::move(publicJwtKey)}
        , subscriptions{}
        , writeGuard{}
        , pendingMessages{}
        , writeInProgress{false}
    {}
    // #####################################################################################################################
    ControlSession::ControlSession(
        std::string sessionId,
        std::weak_ptr<PageAndControlProvider> PageAndControlProvider,
        std::shared_ptr<Roar::WebsocketSession> ws,
        std::function<void()> endSelf,
        std::string publicJwtKey)
        : impl_{std::make_unique<Implementation>(
              std::move(sessionId),
              std::move(PageAndControlProvider),
              std::move(ws),
              std::move(endSelf),
              std::move(publicJwtKey))}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::doRead()
    {
        impl_->ws->read_some()
            .then([weak = weak_from_this()](Roar::WebsocketReadResult readResult) {
                auto self = weak.lock();
                if (!self)
                    return;

                self->onRead(readResult);
            })
            .fail([weak = weak_from_this()](Roar::Error const& e) {
                auto self = weak.lock();
                if (!self)
                    return;

                std::visit(
                    [&e](auto&& arg) {
                        if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, boost::system::error_code>)
                        {
                            if (arg.value() == boost::asio::ssl::error::stream_errors::stream_truncated)
                                return;
                        }
                        spdlog::error("Control session read failed: {}", e.toString());
                    },
                    e.error);
                self->impl_->endSelf();
            });
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::optional<PublisherToken> ControlSession::verifyPublisherIdentity(std::string const& token) const
    {
        return verifyPublisherToken(token, impl_->publicJwtKey);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::setup(std::string const& identity)
    {
        impl_->identity = identity;
        auto publisher = getAssociatedPublisher();
        publisher->setCurrentControlSession(weak_from_this());
        doRead();
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::string ControlSession::identity() const
    {
        return impl_->identity;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::informAboutConnection(std::string const& serviceId, std::string const& tunnelId)
    {
        auto publisher = getAssociatedPublisher();
        auto service = publisher->getService(serviceId);
        if (service == nullptr)
        {
            spdlog::error("Service with id '{}' not found", serviceId);
            return; // TODO: handle error
        }

        auto serviceInfo = service->info();

        spdlog::info("Asking publisher for connection to pipe.");
        writeJson(json{
            {"type", "NewTunnel"},
            {"serviceId", serviceId},
            {"tunnelId", tunnelId},
            {"publicPort", serviceInfo.publicPort},
            {"hiddenPort", serviceInfo.hiddenPort},
            {"socketType", "tcp"}});
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::onRead(Roar::WebsocketReadResult const& readResult)
    {
        spdlog::debug("Control session '{}' received {} bytes.", impl_->identity, readResult.message.size());

        bool abortReading = false;
        auto readAgain = Roar::ScopeExit{[weak = weak_from_this(), &abortReading]() {
            if (abortReading)
                return;

            auto self = weak.lock();
            if (!self)
                return;

            // Read again
            self->doRead();
        }};

        if (!readResult.isBinary)
        {
            impl_->textParser.feed(readResult.message);
            const auto popped = impl_->textParser.popMessage();
            if (!popped)
            {
                spdlog::info("No message popped from parser.");
                return;
            }

            std::string ref = "";
            if (!popped->contains("ref"))
                spdlog::warn("Message lacks ref, cannot reply with ref.");
            else
                ref = (*popped)["ref"].get<std::string>();

            if (!popped->contains("type"))
                return respondWithError(ref, "Type missing in message.");

            const auto msgType = (*popped)["type"].get<std::string>();
            if (msgType != "Ping")
                spdlog::info("'{}': Message '{}' received", impl_->identity, msgType);

            try
            {
                onJson(*popped, ref);
            }
            catch (std::exception const& exc)
            {
                spdlog::error("Error in json consumer: {}", exc.what());
                impl_->endSelf();
                abortReading = true;
                return;
            }
        }
        else
        {
            spdlog::warn("Binary received on control connection, which is ignored.");
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<Publisher> ControlSession::getAssociatedPublisher()
    {
        auto pac = impl_->page_and_control.lock();
        if (!pac)
            return {};

        return pac->obtainPublisher(impl_->identity);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::onJson(json const& j, std::string const& ref)
    {
        if (!j.contains("type") || j["type"].get<std::string>() != "Ping")
            spdlog::info("Control session '{}' received json message.", impl_->identity);
        return impl_->dispatcher.dispatch(j, ref);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::respond(std::string const& ref, json const& j)
    {
        std::string type = j.contains("type") ? j["type"].get<std::string>() : "GenericResponse";

        writeJson(json{
            {"type", type},
            {"ref", ref},
            {"payload", j},
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::respondWithError(std::string const& ref, std::string const& msg)
    {
        spdlog::info("Responding with error: '{}'.");
        writeJson(json{
            {"type", "Error"},
            {"ref", ref},
            {"error", msg},
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    ControlSession::~ControlSession()
    {
        auto publisher = getAssociatedPublisher();
        if (publisher)
            publisher->detachControlSession(true);
        else
            spdlog::error("Publisher is gone, cannot detach control session.");
        spdlog::info("Control session '{}' destroyed.", impl_->identity);
    }
    //---------------------------------------------------------------------------------------------------------------------
    boost::asio::ip::tcp::endpoint ControlSession::remoteEndpoint() const
    {
        return impl_->remoteEndpoint;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::writeOnce()
    {
        std::scoped_lock writeLock{impl_->writeGuard};
        if (impl_->pendingMessages.empty())
        {
            spdlog::debug("No more messages to write on control session.");
            impl_->writeInProgress = false;
            return;
        }
        impl_->writeInProgress = true;

        const auto msg = impl_->pendingMessages.front();
        impl_->pendingMessages.pop_front();

        spdlog::debug(
            "Writing message to control session: '{}'",
            msg.payload.substr(0, std::min(msg.payload.size(), static_cast<std::size_t>(100))));

        impl_->ws->send(msg.payload)
            .then([weak = weak_from_this()](std::size_t) {
                auto self = weak.lock();
                if (!self)
                {
                    spdlog::error("Control session is gone, cannot send message.");
                    return;
                }

                self->writeOnce();
            })
            .fail([weak = weak_from_this()](Roar::Error const& error) {
                auto self = weak.lock();
                if (!self)
                {
                    spdlog::error("Control session is gone, cannot react to send failure.");
                    return;
                }

                spdlog::error("Failed to send message: {}", error.toString());
                self->impl_->endSelf();
            });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::writeJson(json const& j)
    {
        if (!j.contains("type") || j["type"].get<std::string>() != "Pong")
            spdlog::info("Writing json on control session");

        std::scoped_lock writeLock{impl_->writeGuard};
        impl_->pendingMessages.push_back({j.dump()});

        if (!impl_->writeInProgress)
            writeOnce();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::subscribe(
        std::string const& type,
        std::function<bool(Subscription::ParameterType const&, std::string const&)> const& callback)
    {
        spdlog::info("Subscribing to '{}' on control session.", type);
        impl_->subscriptions.push_back(impl_->dispatcher.subscribe(type, callback));
    }
    // #####################################################################################################################
}
