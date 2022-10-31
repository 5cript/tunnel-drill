#include <boost/asio.hpp>

#include <publisherpp/publisher.hpp>

#include <sharedpp/json.hpp>
#include <roar/ssl/make_ssl_context.hpp>
#include <roar/curl/request.hpp>
#include <roar/utility/base64.hpp>
#include <spdlog/spdlog.h>

using namespace std::string_literals;
using namespace std::chrono_literals;

namespace TunnelBore::Publisher
{
    // #####################################################################################################################
    Publisher::Publisher(boost::asio::any_io_executor exec, Config cfg)
        : cfg_{std::move(cfg)}
        , ws_{std::make_shared<Roar::WebsocketClient>(Roar::WebsocketClient::ConstructionArguments{
              .executor = exec,
              .sslContext =
                  [] {
                      boost::asio::ssl::context sslContext{boost::asio::ssl::context::tlsv13_client};
                      sslContext.set_verify_mode(boost::asio::ssl::verify_none);
                      sslContext.set_options(
                          boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 |
                          boost::asio::ssl::context::single_dh_use);
                      return sslContext;
                  }(),
          })}
        , services_{[this, &exec]() {
            std::vector<std::shared_ptr<Service>> services;
            for (auto const& serviceInfo : cfg_.services)
            {
                services.push_back(std::make_shared<Service>(
                    exec,
                    serviceInfo.name,
                    serviceInfo.publicPort,
                    serviceInfo.hiddenHost ? *serviceInfo.hiddenHost : "localhost",
                    serviceInfo.hiddenPort));
            }
            return services;
        }()}
        , authToken_{}
        , tokenCreationTime_{}
        , reconnectTimer_{exec}
        , reconnectTime_{1}
        , isReconnecting_{false}
        , reconnectMutex_{}
        , controlSendQueueMutex_{}
        , controlSendOperations_{}
        , sendInProgress_{false}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    void Publisher::authenticate()
    {
        const auto response =
            Roar::Curl::Request{}
                .basicAuth(cfg_.identity, cfg_.passHashed)
                .verifyPeer(false)
                .verifyHost(false)
                .sink(authToken_)
                .get("https://"s + cfg_.authorityHost + ":"s + std::to_string(cfg_.authorityPort) + "/api/auth"s);

        if (response.code() != boost::beast::http::status::ok)
        {
            spdlog::error("Failed to authenticate with the authority. Response code: {}", static_cast<int>(response.code()));
            retryConnect();
            return;
        }
        spdlog::info("Authentication successful");

        tokenCreationTime_ = std::chrono::system_clock::now();

        connectToBroker();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Publisher::retryConnect()
    {
        std::scoped_lock lock{reconnectMutex_};
        if (isReconnecting_)
            return;
        spdlog::info("Retrying connection in {} seconds", reconnectTime_.count());
        isReconnecting_ = true;
        reconnectTimer_.expires_from_now(boost::posix_time::seconds(reconnectTime_.count()));
        reconnectTime_ *= 2;
        if (reconnectTime_.count() > 60)
            reconnectTime_ = 60s;
        reconnectTimer_.async_wait([weak = weak_from_this()](boost::system::error_code ec) {
            if (ec)
                return;
            auto self = weak.lock();
            if (!self)
                return;
            
            std::scoped_lock lock{self->reconnectMutex_};
            self->isReconnecting_ = false;
            self->authenticate();
        });
        
        connectToBroker();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Publisher::connectToBroker()
    {
        spdlog::info("Connecting to broker control line '{}:{}'", cfg_.host, cfg_.port);
        ws_->connect({
                         .host = cfg_.host,
                         .port = std::to_string(cfg_.port),
                         .path = "/api/ws/publisher",
                         .timeout = std::chrono::seconds{5},
                         .headers = {{
                             boost::beast::http::field::authorization,
                             "Bearer "s + Roar::base64Encode(authToken_),
                         }},
                     })
            .then([weak = weak_from_this()]() {
                auto self = weak.lock();
                if (!self)
                    return;

                spdlog::info("Connected to broker control line");
                {
                    std::scoped_lock lock{self->reconnectMutex_};
                    self->reconnectTime_ = 1s;
                }
                self->doControlReading();
                json handshake = {
                    {"type", "Handshake"}, {"identity", self->cfg_.identity}, {"services", self->services_}};
                self->sendQueued(std::move(handshake));
            })
            .fail([](auto&& err) {
                spdlog::error("Failed to connect to broker: {}", err.toString());
            });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Publisher::doControlReading()
    {
        ws_->read()
            .then([weak = weak_from_this()](auto msg) {
                auto self = weak.lock();
                if (!self)
                    return;

                self->onControlRead(std::move(msg));
                self->doControlReading();
            })
            .fail([weak = weak_from_this()](auto const& err) {
                spdlog::error("Failed to read from broker: '{}'", err.toString());
                auto self = weak.lock();
                if (!self)
                    return;
                self->retryConnect();
            });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Publisher::sendOnceFromQueue()
    {
        std::scoped_lock lock{controlSendQueueMutex_};
        if (controlSendOperations_.empty())
        {
            sendInProgress_ = false;
            return;
        }
        sendInProgress_ = true;

        auto op = std::move(controlSendOperations_.front());
        controlSendOperations_.pop_front();

        ws_->send(op.payload.dump())
            .then([cb = std::move(op.callback), weak = weak_from_this()](auto&&) {
                cb();

                auto self = weak.lock();
                if (!self)
                    return;

                self->sendOnceFromQueue();
            })
            .fail([](auto const& err) {
                spdlog::error("Failed to send to broker: '{}'", err.toString());
                // TODO: Am i still connected? If not reconnect?
            });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Publisher::sendQueued(json&& j)
    {
        std::scoped_lock lock{controlSendQueueMutex_};
        controlSendOperations_.emplace_back(ControlSendOperation{std::move(j), []() {}});
        if (!sendInProgress_)
            sendOnceFromQueue();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Publisher::onControlRead(Roar::WebsocketReadResult message)
    {
        json j = json::parse(message.message);
        const auto type = j["type"].get<std::string>();
        if (type == "NewTunnel")
        {
            spdlog::info("Received NewTunnel message from broker");
            onNewTunnel(
                j["serviceId"].get<std::string>(),
                j["tunnelId"].get<std::string>(),
                j["hiddenPort"].get<int>(),
                j["publicPort"].get<int>(),
                j["socketType"].get<std::string>());
        }
        else if (type == "Error")
        {
            spdlog::error("Received Error message from broker: {}", j.dump());
        }
        else
        {
            spdlog::error("Received unknown message from broker: {}", j.dump());
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Publisher::onNewTunnel(
        std::string const& serviceId,
        std::string const& tunnelId,
        int hiddenPort,
        int publicPort,
        std::string const& socketType)
    {
        spdlog::info("Creating new tunnel for service '{}' with id '{}'", serviceId, tunnelId);

        auto respondWithFailure = [&](std::string const& reason) {
            sendQueued({
                {"type", "NewTunnelFailed"},
                {"serviceId", serviceId},
                {"tunnelId", tunnelId},
                {"hiddenPort", hiddenPort},
                {"publicPort", publicPort},
                {"socketType", socketType},
                {"reason", reason},
            });
        };

        auto service = std::find_if(services_.begin(), services_.end(), [&](auto const& service) {
            return service->hiddenPort() == hiddenPort && service->publicPort() == publicPort;
        });
        if (service == services_.end())
        {
            spdlog::error("Received NewTunnel message for unknown service '{}'", serviceId);
            respondWithFailure("Unknown service");
            return;
        }

        std::string body;
        const auto response = Roar::Curl::Request{}
                                  .verifyPeer(false)
                                  .verifyHost(false)
                                  .basicAuth(cfg_.identity, cfg_.passHashed)
                                  .source(json{
                                      {"tunnelId", tunnelId},
                                      {"serviceId", serviceId},
                                      {"hiddenPort", hiddenPort},
                                      {"publicPort", publicPort}}
                                              .dump())
                                  .sink(body)
                                  .post(
                                      "https://"s + cfg_.authorityHost + ":" + std::to_string(cfg_.authorityPort) +
                                      "/api/auth/sign-json");

        if (response.code() != boost::beast::http::status::ok)
        {
            spdlog::error("Failed to sign tunnel request: {}", body);
            respondWithFailure("Failed to sign tunnel request");
            return;
        }
        const auto tunnelToken = json::parse(body)["token"].get<std::string>();
        (*service)->createSession(cfg_.host, tunnelToken, tunnelId);
    }
    //---------------------------------------------------------------------------------------------------------------------
    Publisher::~Publisher()
    {}
    // #####################################################################################################################
}