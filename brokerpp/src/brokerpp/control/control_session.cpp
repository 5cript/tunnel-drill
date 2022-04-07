#include <brokerpp/control/control_session.hpp>
#include <brokerpp/control/stream_parser.hpp>
#include <brokerpp/control/dispatcher.hpp>
#include <brokerpp/control/user.hpp>

#include <spdlog/spdlog.h>

namespace TunnelBore::Broker
{
    //#####################################################################################################################
    struct ControlSession::Implementation
    {
        std::string sessionId;
        bool authenticated;
        std::weak_ptr<ControlSession> session;
        StreamParser textParser;
        Dispatcher dispatcher;
        User user;
        
        void imbueOwner(std::weak_ptr<ControlSession> session);

        Implementation(std::string sessionId);
    };
    //---------------------------------------------------------------------------------------------------------------------
    ControlSession::Implementation::Implementation(std::string sessionId)
        : sessionId{std::move(sessionId)}
        , authenticated{false}
        , session{}
        , textParser{}
        , dispatcher{}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::Implementation::imbueOwner(std::weak_ptr<ControlSession> sess)
    {
        session = sess;
    }
    //#####################################################################################################################
    ControlSession::ControlSession(
        attender::websocket::connection* owner,
        std::string sessionId
    )
        : attender::websocket::session_base{owner}
        , impl_{std::make_unique<Implementation>(sessionId)}
    {
    }
    //---------------------------------------------------------------------------------------------------------------------
    ControlSession::~ControlSession() = default;
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::setup()
    {
        impl_->imbueOwner(weak_from_this());
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
            return respondWithError((*popped)["ref"].get<int>(), "Type missing in message.");

        spdlog::info("Message '{}' received", (*popped)["type"].get<std::string>());

        try
        {
            onJson(*popped, ref);
        }
        catch (std::exception const& exc)
        {
            spdlog::error("Error in json consumer: {}", exc.what());
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    void ControlSession::onJson(json const& j, std::string const& ref)
    {
        if (impl_->authenticated)
            return impl_->dispatcher.dispatch(j, ref);

        if (impl_->user.authenticate(j["payload"]))
        {
            impl_->authenticated = true;
            writeJson(
                json{
                    {"ref", ref},
                    {"authenticated", true},
                },
                [this](auto, auto) {
                    onAfterAuthentication();
                });
        }
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
    void ControlSession::respondWithError(int ref, std::string const& msg)
    {
        spdlog::info("Responding with error: '{}'.");
        writeJson(json{
            {"ref", ref},
            {"error", msg},
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool ControlSession::writeText(
        std::string const& txt,
        std::function<void(session_base*, std::size_t)> const& on_complete)
    {
        std::stringstream sstr;
        sstr << "0x" << std::hex << std::setw(8) << std::setfill('0') << txt.size() << "|" << txt;
        return write_text(sstr.str(), on_complete);
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
    //#####################################################################################################################
    
}