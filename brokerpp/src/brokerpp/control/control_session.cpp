#include <brokerpp/control/control_session.hpp>
#include <brokerpp/control/stream_parser.hpp>
#include <brokerpp/control/dispatcher.hpp>
#include <brokerpp/controller.hpp>
#include <brokerpp/publisher/service.hpp>

#include <spdlog/spdlog.h>

#include <iomanip>
#include <sstream>

namespace TunnelBore::Broker
{
    //#####################################################################################################################
    struct ControlSession::Implementation
    {
        std::string identity;
        std::string sessionId;
        bool authenticated;
        std::weak_ptr<Controller> controller;
        StreamParser textParser;
        Dispatcher dispatcher;
        boost::asio::ip::tcp::endpoint remoteEndpoint;

        std::vector<std::shared_ptr<Subscription>> subscriptions;

        Implementation(std::string sessionId, std::weak_ptr<Controller> controller);
    };
    //---------------------------------------------------------------------------------------------------------------------
    ControlSession::Implementation::Implementation(std::string sessionId, std::weak_ptr<Controller> controller)
        : identity{}
        , sessionId{std::move(sessionId)}
        , authenticated{false}
        , controller{controller}
        , textParser{}
        , dispatcher{}
        , remoteEndpoint{}
    {}
    //#####################################################################################################################
    ControlSession::ControlSession(
        attender::websocket::connection* owner,
        std::weak_ptr<Controller> controller,
        std::string sessionId
    )
        : attender::websocket::session_base{owner}
        , impl_{std::make_unique<Implementation>(sessionId, controller)}
    {
        owner->with_stream_do([this](auto& stream)
        {
            impl_->remoteEndpoint = boost::beast::get_lowest_layer(stream).socket().remote_endpoint();
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    boost::asio::ip::tcp::endpoint ControlSession::remoteEndpoint() const
    {
        return impl_->remoteEndpoint;
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<Publisher> ControlSession::getAssociatedPublisher()
    {
        auto controller = impl_->controller.lock();
        if (!controller)
            return {};

        return controller->obtainPublisher(impl_->identity);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::subscribe(
        std::string const& type, 
        std::function<bool(Subscription::ParameterType const&, std::string const&)> const& callback
    )
    {
        impl_->subscriptions.push_back(impl_->dispatcher.subscribe(type, callback));
    }
    //---------------------------------------------------------------------------------------------------------------------
    ControlSession::~ControlSession() = default;
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
            return;

        auto serviceInfo = service->info();

        spdlog::info("Asking publisher for connection to pipe.");
        writeJson(json{
            {"type", "NewTunnel"},
            {"serviceId", serviceId},
            {"tunnelId", tunnelId},
            {"publicPort", serviceInfo.publicPort},
            {"hiddenPort", serviceInfo.hiddenPort},
            {"socketType", "tcp"}
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::on_text(std::string_view txt)
    {
        impl_->textParser.feed(txt);
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
            endSession();
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::onJson(json const& j, std::string const& ref)
    {
        return impl_->dispatcher.dispatch(j, ref);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::endSession()
    {
        close_connection();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::onAfterAuthentication()
    {
        try
        {
            // writeJson(json{
            //     {"ref", -1},
            //     {"pluginsLoaded", pluginNames},
            // });
        }
        catch (std::exception const& exc)
        {
            spdlog::error("On after authentication error: '{}'", exc.what());
            writeJson(
                json{
                    {"ref", -1},
                    {"error", exc.what()},
                },
                [this](auto, auto) {
                    endSession();
                }
            );
        }
        catch (...)
        {
            spdlog::error("On after authentication error (nostdexcept)");
            writeJson(
                json{
                    {"ref", -1},
                    {"error", "Non standard exception caught."},
                },
                [this](auto, auto) {
                    endSession();
                }
            );
        }
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
    bool ControlSession::writeText(
        std::string const& txt,
        std::function<void(session_base*, std::size_t)> const& on_complete)
    {
        return write_text(txt, on_complete);
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool
    ControlSession::writeJson(json const& j, std::function<void(session_base*, std::size_t)> const& on_complete)
    {
        const std::string serialized = j.dump();
        return writeText(serialized, on_complete);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::on_write_complete(std::size_t bytesTransferred)
    {
        session_base::on_write_complete(bytesTransferred);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::on_binary(char const*, std::size_t)
    {
        spdlog::warn("Binary was received, but cannot handle binary data.");
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::on_close()
    {
        spdlog::info("Control session was closed: {}", impl_->sessionId);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::on_error(boost::system::error_code ec, char const*)
    {
        spdlog::info("Error in control session: {}.", ec.message());
    }
    //#####################################################################################################################
    
}