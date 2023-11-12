#include <brokerpp/publisher/tunnel_session.hpp>
#include <brokerpp/publisher/service.hpp>
#include <brokerpp/control/control_session.hpp>
#include <sharedpp/json.hpp>
#include <sharedpp/pipe_operation.hpp>
#include <sharedpp/constants.hpp>
#include <sharedpp/printable_string.hpp>

#include <spdlog/spdlog.h>
#include <roar/utility/scope_exit.hpp>

#include <iterator>
#include <iostream>

namespace TunnelBore::Broker
{
    namespace
    {
        struct PublisherConnectionMessage
        {
            std::string identity;
            std::string tunnelId;
            std::string serviceId;
            unsigned short hiddenPort;
            unsigned short publicPort;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_EX(
            PublisherConnectionMessage,
            identity,
            tunnelId,
            serviceId,
            hiddenPort,
            publicPort)
    }
    // #####################################################################################################################
    struct TunnelSession::Implementation
    {
        std::recursive_mutex closeLock;
        std::recursive_mutex peekBufferLock;
        boost::asio::ip::tcp::socket socket;
        std::weak_ptr<ControlSession> controlSession;
        std::string peekBuffer;
        bool isPublisherSide;
        std::string tunnelId;
        std::weak_ptr<Service> service;
        boost::asio::deadline_timer inactivityTimer;
        std::atomic_bool wasClosed;
        std::string remoteAddress;
        std::shared_ptr<PipeOperation<TunnelSession>> pipeOperation;

        Implementation(
            boost::asio::ip::tcp::socket&& socket,
            std::string tunnelId,
            std::weak_ptr<ControlSession> controlSession,
            std::weak_ptr<Service> service)
            : closeLock{}
            , socket{std::move(socket)}
            , controlSession{std::move(controlSession)}
            , peekBuffer(4096, '\0')
            , isPublisherSide{false}
            , tunnelId{std::move(tunnelId)}
            , service{std::move(service)}
            , inactivityTimer{socket.get_executor()}
            , wasClosed{false}
            , remoteAddress{[this]() {
                auto const& endpoint = this->socket.remote_endpoint();
                auto const& address = endpoint.address();
                auto const& port = endpoint.port();
                return address.to_string() + ":" + std::to_string(port);
            }()}
            , pipeOperation{}
        {}
    };
    // #####################################################################################################################
    TunnelSession::TunnelSession(
        boost::asio::ip::tcp::socket&& socket,
        std::string tunnelId,
        std::weak_ptr<ControlSession> controlSession,
        std::weak_ptr<Service> service)
        : impl_{std::make_unique<Implementation>(
              std::move(socket),
              std::move(tunnelId),
              std::move(controlSession),
              std::move(service))}
    {
        spdlog::info("Tunnel side created for '{}'", impl_->remoteAddress);
    }
    //---------------------------------------------------------------------------------------------------------------------
    boost::asio::ip::tcp::socket& TunnelSession::socket()
    {
        return impl_->socket;
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::string TunnelSession::id() const
    {
        return impl_->tunnelId;
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::string TunnelSession::remoteAddress() const
    {
        return impl_->remoteAddress;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void TunnelSession::resetTimer()
    {
        std::scoped_lock lock{impl_->closeLock};
        try
        {
            impl_->inactivityTimer.expires_from_now(boost::posix_time::seconds(InactivityTimeout.count()));
            impl_->inactivityTimer.async_wait([weak = weak_from_this()](auto const& ec) {
                if (ec == boost::asio::error::operation_aborted)
                    return;

                auto self = weak.lock();
                if (!self)
                {
                    spdlog::warn("Tunnel is gone in inactivity timer timeout");
                    return;
                }

                if (ec)
                {
                    spdlog::warn(
                        "Inactivity timer for tunnel '{}' error: '{}'.", self->impl_->remoteAddress, ec.message());
                    self->close();
                    return;
                }

                // Check whether the deadline has passed. We compare the deadline against
                // the current time since a new asynchronous operation may have moved the
                // deadline before this actor had a chance to run.
                std::scoped_lock lock{self->impl_->closeLock};
                if (self->impl_->inactivityTimer.expires_at() <= boost::asio::deadline_timer::traits_type::now())
                {
                    spdlog::warn("Closing tunnel '{}' due to inactivity.", self->impl_->remoteAddress);
                    self->close();
                }
            });
        }
        catch (std::exception const& exc)
        {
            spdlog::error(
                "Exception in attempt to reset inactivity timer for tunnel '{}': '{}'",
                impl_->remoteAddress,
                exc.what());
            close();
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    void TunnelSession::peek()
    {
        resetTimer();

        try
        {
            impl_->socket.async_read_some(
                boost::asio::buffer(impl_->peekBuffer, impl_->peekBuffer.size()),
                [weak = weak_from_this()](const boost::system::error_code& ec, std::size_t bytesTransferred) {
                    auto exitLog = Roar::ScopeExit{[]() {
                        spdlog::info("Peek read finished.");
                    }};

                    spdlog::info("Peek read for tunnel side of size '{}'.", bytesTransferred);

                    auto self = weak.lock();
                    if (!self)
                    {
                        spdlog::warn("Tunnel is gone in peek read");
                        return;
                    }

                    auto service = self->impl_->service.lock();
                    if (!service)
                    {
                        spdlog::warn(
                            "Missing service for new tunnel session, this will terminate this tunnel '{}'",
                            self->impl_->remoteAddress);
                        self->close();
                        return;
                    }

                    auto info = service->info();

                    if (ec)
                    {
                        spdlog::warn(
                            "Initial read for tunnel side failed, for service '{}:{}->{}', this will terminate this "
                            "side "
                            "of the tunnel '{}': '{}'",
                            info.name ? *info.name : "noname",
                            info.publicPort,
                            info.hiddenPort,
                            self->impl_->remoteAddress,
                            ec.message());
                        self->close();
                        return;
                    }

                    auto controlSession = self->impl_->controlSession.lock();
                    if (!controlSession)
                    {
                        spdlog::warn(
                            "Missing control session for new session, this will terminate this tunnel '{}'",
                            self->impl_->remoteAddress);
                        self->close();
                        return;
                    }

                    if (bytesTransferred == 0)
                    {
                        spdlog::warn(
                            "Initial read for tunnel side failed, for service '{}:{}->{}', this will terminate this "
                            "side "
                            "of the tunnel '{}': '{}'",
                            info.name ? *info.name : "noname",
                            info.publicPort,
                            info.hiddenPort,
                            self->impl_->remoteAddress,
                            "No bytes transferred");
                        self->close();
                        return;
                    }

                    if (self->impl_->peekBuffer.starts_with(publisherToBrokerPrefix))
                    {
                        try
                        {
                            if (self->impl_->peekBuffer.size() < publisherToBrokerPrefix.size() + 1)
                            {
                                spdlog::warn(
                                    "Invalid initial message for tunnel side, this will terminate this side of the "
                                    "tunnel "
                                    "'{}'",
                                    self->impl_->remoteAddress);
                                self->close();
                                return;
                            }

                            self->impl_->peekBuffer = self->impl_->peekBuffer.substr(
                                publisherToBrokerPrefix.size() + 1,
                                bytesTransferred - publisherToBrokerPrefix.size() - 1);

                            // {identity, tunnelId, serviceId, hiddenPort, publicPort}
                            auto token = controlSession->verifyPublisherIdentity(self->impl_->peekBuffer);
                            self->impl_->peekBuffer.clear();

                            if (!token)
                            {
                                spdlog::warn(
                                    "Invalid publisher identity, this will terminate this tunnel '{}'.",
                                    self->impl_->remoteAddress);
                                self->close();
                                return;
                            }

                            self->impl_->isPublisherSide = true;

                            if (token->identity() != controlSession->identity())
                            {
                                spdlog::warn(
                                    "Received tunnel info from another remote than the control socket. This is not "
                                    "allowed. Tunnel identity: '{}', tunnel address: '{}', control identity: '{}'.",
                                    token->identity(),
                                    self->impl_->remoteAddress,
                                    controlSession->identity());
                                self->close();
                                return;
                            }

                            service->connectTunnels(
                                token->claims()["tunnelId"].get<std::string>(), self->impl_->tunnelId);
                            return;
                        }
                        catch (std::exception const& exc)
                        {
                            spdlog::warn(
                                "Exception in attempt to parse tunnel '{}' connection from publisher: '{}'",
                                self->impl_->remoteAddress,
                                exc.what());
                            self->close();
                            return;
                        }
                    }
                    else
                    {
                        self->impl_->peekBuffer.resize(bytesTransferred);
                        spdlog::info(
                            "Connection '{}' for service '{}:{}->{}' does not look like publisher side. Bytes received "
                            "'{}', "
                            "Starting with '{}'.",
                            self->impl_->remoteAddress,
                            info.name ? *info.name : "noname",
                            info.publicPort,
                            info.hiddenPort,
                            bytesTransferred,
                            makePrintableString(
                                self->impl_->peekBuffer.substr(0, std::min(std::size_t{24}, bytesTransferred))));
                    }

                    // assume this is not json from the publisher side.
                    self->impl_->isPublisherSide = false;
                    spdlog::info("Informing publisher about connection");
                    controlSession->informAboutConnection(service->serviceId(), self->impl_->tunnelId);
                });
        }
        catch (std::exception const& exc)
        {
            spdlog::error("Exception in attempt to read from tunnel '{}': '{}'", impl_->remoteAddress, exc.what());
            close();
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    void TunnelSession::link(TunnelSession& other)
    {
        ServiceInfo info{std::nullopt, 0, 0};
        auto service = impl_->service.lock();
        if (service)
            info = service->info();

        spdlog::info(
            "Connecting tunnel '{}' with '{}' for service '{}:{}->{}'.",
            impl_->remoteAddress,
            other.impl_->remoteAddress,
            info.name ? *info.name : "noname",
            info.publicPort,
            info.hiddenPort);

        // Self -> Other
        if (!impl_->peekBuffer.empty())
        {
            try
            {
                boost::asio::async_write(
                    other.impl_->socket,
                    boost::asio::buffer(impl_->peekBuffer),
                    [self = shared_from_this(), otherSelf = other.shared_from_this()](auto const& ec, std::size_t) {
                        if (ec)
                        {
                            spdlog::warn(
                                "Failed to write initial data to tunnel '{}', this will terminate this tunnel '{}'.",
                                otherSelf->impl_->remoteAddress,
                                self->impl_->remoteAddress);
                            self->close();
                            otherSelf->close();
                            return;
                        }
                        self->impl_->pipeOperation = self->pipeTo(*otherSelf);
                    });
            }
            catch (std::exception const& exc)
            {
                spdlog::error("Exception in attempt to write to tunnel '{}': '{}'", impl_->remoteAddress, exc.what());
                close();
            }
        }
        else
            impl_->pipeOperation = pipeTo(other);

        // Other -> Self
        if (!other.impl_->peekBuffer.empty())
        {
            try
            {
                boost::asio::async_write(
                    impl_->socket,
                    boost::asio::buffer(other.impl_->peekBuffer),
                    [self = shared_from_this(), otherSelf = other.shared_from_this()](auto const& ec, std::size_t) {
                        if (ec)
                        {
                            spdlog::warn(
                                "Failed to write initial data to tunnel '{}', this will terminate this tunnel '{}'.",
                                self->impl_->remoteAddress,
                                otherSelf->impl_->remoteAddress);
                            self->close();
                            otherSelf->close();
                            return;
                        }
                        otherSelf->impl_->pipeOperation = otherSelf->pipeTo(*self);
                    });
            }
            catch (std::exception const& exc)
            {
                spdlog::error(
                    "Exception in attempt to write to tunnel '{}': '{}'", other.impl_->remoteAddress, exc.what());
                other.close();
            }
        }
        else
            other.impl_->pipeOperation = other.pipeTo(*this);
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<PipeOperation<TunnelSession>> TunnelSession::pipeTo(TunnelSession& other)
    {
        auto pipeOperation = std::make_shared<PipeOperation<TunnelSession>>(this, &other);
        pipeOperation->doPipe();
        return pipeOperation;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void TunnelSession::delayedClose()
    {
        auto delayTimer = std::make_shared<boost::asio::deadline_timer>(impl_->socket.get_executor());
        delayTimer->expires_from_now(boost::posix_time::seconds{3});
        impl_->inactivityTimer.async_wait([weak = weak_from_this(), delayTimer](auto const& ec) {
            if (ec == boost::asio::error::operation_aborted)
                return;

            auto self = weak.lock();
            if (!self)
                return;

            self->close();
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void TunnelSession::close()
    {
        std::scoped_lock lock{impl_->closeLock};

        if (impl_->wasClosed)
            return;
        impl_->wasClosed = true;

        spdlog::info("Closing tunnel session '{}'.", impl_->remoteAddress);

        boost::system::error_code ignore;
        impl_->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);

        impl_->inactivityTimer.cancel();

        auto service = impl_->service.lock();
        if (!service)
            return;

        service->closeTunnelSide(impl_->tunnelId);
    }
    //---------------------------------------------------------------------------------------------------------------------
    TunnelSession::~TunnelSession()
    {
        spdlog::info("Tunnel side destroyed for '{}'", impl_->remoteAddress);
        close();
    }
    //---------------------------------------------------------------------------------------------------------------------
    TunnelSession::TunnelSession(TunnelSession&&) = default;
    //---------------------------------------------------------------------------------------------------------------------
    TunnelSession& TunnelSession::operator=(TunnelSession&&) = default;
    // #####################################################################################################################
}