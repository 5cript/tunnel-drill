#include <brokerpp/winsock_first.hpp>
#include <brokerpp/control/control_session.hpp>
#include <brokerpp/control/dispatcher.hpp>
#include <brokerpp/request_listener/page_control_provider.hpp>
#include <brokerpp/publisher/service.hpp>
#include <brokerpp/control/stream_parser.hpp>
#include <brokerpp/publisher/publisher.hpp>

#include <spdlog/spdlog.h>

#include <string>

namespace TunnelBore::Broker
{
    //#####################################################################################################################
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

        std::vector<std::shared_ptr<Subscription>> subscriptions;

        Implementation(
            std::string sessionId,
            std::weak_ptr<PageAndControlProvider> PageAndControlProvider,
            std::shared_ptr<Roar::WebsocketSession> ws,
            std::function<void()> endSelf);
    };
    //---------------------------------------------------------------------------------------------------------------------
    ControlSession::Implementation::Implementation(
        std::string sessionId,
        std::weak_ptr<PageAndControlProvider> PageAndControlProvider,
        std::shared_ptr<Roar::WebsocketSession> ws,
        std::function<void()> endSelf)
        : sessionId{std::move(sessionId)}
        , page_and_control{std::move(PageAndControlProvider)}
        , ws{std::move(ws)}
        , identity{}
        , authenticated{false}
        , textParser{}
        , dispatcher{}
        , remoteEndpoint{}
        , endSelf{std::move(endSelf)}
        , subscriptions{}
    {}
    //#####################################################################################################################
    ControlSession::ControlSession(
        std::string sessionId,
        std::weak_ptr<PageAndControlProvider> PageAndControlProvider,
        std::shared_ptr<Roar::WebsocketSession> ws,
        std::function<void()> endSelf)
        : impl_{std::make_unique<Implementation>(
              std::move(sessionId),
              std::move(PageAndControlProvider),
              std::move(ws),
              std::move(endSelf))}
    {
        doRead();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::doRead()
    {
        impl_->ws->read()
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

                spdlog::error("Control session read failed: {}", e.toString());
                self->impl_->endSelf();
            });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::setup(std::string const& identity)
    {
        impl_->identity = identity;
        auto publisher = getAssociatedPublisher();
        publisher->setCurrentControlSession(weak_from_this());
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
        if (!service)
            return; // TODO: handle error

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
        if (!readResult.isBinary)
        {
            impl_->textParser.feed(readResult.message);
            const auto popped = impl_->textParser.popMessage();
            if (!popped)
                return;

            std::string ref = "";
            if (!popped->contains("ref"))
                spdlog::warn("Message lacks ref, cannot reply with ref.");
            else
                ref = (*popped)["ref"];

            if (!popped->contains("type"))
                return respondWithError((*popped)["ref"].get<std::string>(), "Type missing in message.");

            spdlog::info("Message '{}' received", (*popped)["type"].get<std::string>());

            try
            {
                onJson(*popped, ref);
            }
            catch (std::exception const& exc)
            {
                spdlog::error("Error in json consumer: {}", exc.what());
                impl_->endSelf();
            }
        }
        else
        {
            spdlog::warn("Binary received on control connection, which is ignored.");
        }

        // Restart reading:
        doRead();
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
        return impl_->dispatcher.dispatch(j, ref);
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
    ControlSession::~ControlSession() = default;
    //---------------------------------------------------------------------------------------------------------------------
    boost::asio::ip::tcp::endpoint ControlSession::remoteEndpoint() const
    {
        return impl_->remoteEndpoint;
    }
    //---------------------------------------------------------------------------------------------------------------------
    Roar::Detail::PromiseTypeBind<
        Roar::Detail::PromiseTypeBindThen<std::size_t>,
        Roar::Detail::PromiseTypeBindFail<Roar::Error const&>>
    ControlSession::writeJson(json const& j)
    {
        return promise::newPromise([this, &j](promise::Defer d) {
            impl_->ws->send(j.dump())
                .then([d](std::size_t amount) {
                    d.resolve(amount);
                })
                .fail([d, weak = weak_from_this()](Roar::Error const& error) {
                    auto self = weak.lock();
                    if (!self)
                        return d.reject(error);

                    spdlog::error("Failed to send message: {}", error.toString());
                    self->impl_->endSelf();
                    d.reject(error);
                });
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::subscribe(
        std::string const& type,
        std::function<bool(Subscription::ParameterType const&, std::string const&)> const& callback)
    {
        impl_->subscriptions.push_back(impl_->dispatcher.subscribe(type, callback));
    }
    //#####################################################################################################################
}
