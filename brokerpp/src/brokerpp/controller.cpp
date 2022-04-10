#include <brokerpp/controller.hpp>
#include <brokerpp/json.hpp>
#include <brokerpp/control/control_session.hpp>
#include <brokerpp/load_home_file.hpp>

#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/traits.h>
#include <attender/encoding/base64.hpp>
#include <spdlog/spdlog.h>

using namespace std::literals;

namespace TunnelBore::Broker
{
//#####################################################################################################################
Controller::Controller(boost::asio::io_context& context, std::function <void(boost::system::error_code)> on_error)
    : context_{&context}
    , ws_{&context, std::move(on_error), attender::websocket::security_parameters{
        .key = loadHomeFile("key.pem"),
        .cert = loadHomeFile("cert.pem")
    }}
{
}
//---------------------------------------------------------------------------------------------------------------------
Controller::~Controller()
{
    ws_.stop();
}
//---------------------------------------------------------------------------------------------------------------------
std::shared_ptr<Publisher> Controller::obtainPublisher(std::string const& identity)
{
    auto pubIter = publishers_.find(identity);
    if (pubIter == publishers_.end())
    {
        auto publisher = std::make_shared<Publisher>(context_, identity);
        publishers_[identity] = publisher;
        return publisher;
    }
    return pubIter->second;
}
//---------------------------------------------------------------------------------------------------------------------
void Controller::start(unsigned short port)
{
    const auto publicJwt = loadHomeFile("jwt/public.key");
    ws_.start([weak = weak_from_this(), publicJwt](std::shared_ptr<attender::websocket::connection> connection){
        auto shared = weak.lock();
        if (!shared)
        {
            spdlog::error("Connection attempt received after server is dead.");
            return;
        }

        std::scoped_lock lock{shared->guard_};
        spdlog::info("New controller connection.");

        const auto id = shared->generator_.generate_id();
        ControlSession& session = connection->create_session<ControlSession>(weak, id);

        auto closeWithFailure = [&session, &connection](std::string const& reason){
            spdlog::warn("Authorization token declined: '{}'.", reason);
            session.writeJson(json{
                {"type", "Error"},
                {"message", "Unauthorized: "s + reason}
            });
            connection->close();
        };

        const auto& req = connection->get_upgrade_request();
        auto authField = req.find(boost::beast::http::field::authorization);
        if (authField == std::end(req))
        {
            return closeWithFailure("Missing authorization header field.");
        }
        else
        {
            try 
            {
                const auto auth = std::string{authField->value()};
                const auto spacePos = auth.find(" ");
                if (spacePos == std::string::npos || spacePos == auth.size() - 1)
                    return closeWithFailure("Expected space in authorization header field value");

                auto type = std::string_view(auth.c_str(), spacePos);
                auto value = std::string_view(auth.c_str() + spacePos + 1, auth.size() - spacePos - 1);

                if (type != "Bearer")
                    return closeWithFailure("Expected bearer scheme.");

                std::string token;
                attender::base64_decode(value, token);

                const auto decoded = jwt::decode<jwt::traits::nlohmann_json>(token);

                const auto verifier = 
                    jwt::verify<jwt::traits::nlohmann_json>()
                        .allow_algorithm(jwt::algorithm::rs256{publicJwt, "", "", ""})
                        .with_issuer("tunnelBore")
                ;
                std::error_code ec;
                verifier.verify(decoded, ec);
                if (ec)
                    return closeWithFailure("Token was rejected.");

                const auto claims = decoded.get_payload_claims();
                auto ident = claims.find("identity");
                if (ident == std::end(claims))
                    return closeWithFailure("Token lacks identity claim.");

                session.setup(ident->second.to_json().get<std::string>());
            }
            catch(std::exception const& exc)
            {
                return closeWithFailure("Exception thrown in bearer authentication: "s  + exc.what());
            }
        }

        shared->connections_[id] = std::move(connection);
    }, std::to_string(port));
}
//---------------------------------------------------------------------------------------------------------------------
void Controller::stop()
{
    ws_.stop();
}
//#####################################################################################################################
}