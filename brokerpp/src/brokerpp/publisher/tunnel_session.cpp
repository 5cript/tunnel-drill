#include <brokerpp/publisher/tunnel_session.hpp>
#include <brokerpp/publisher/service.hpp>
#include <brokerpp/control/control_session.hpp>
#include <brokerpp/json.hpp>

#include <spdlog/spdlog.h>

#include <iterator>

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

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PublisherConnectionMessage, identity, tunnelId, serviceId, hiddenPort, publicPort)
    }
//#####################################################################################################################
    struct TunnelSession::Implementation
    {
        boost::asio::ip::tcp::socket socket;
        std::weak_ptr<ControlSession> controlSession;
        std::string peekBuffer;
        bool isPublisherSide;
        std::string tunnelId;
        std::weak_ptr<Service> service;
        boost::asio::deadline_timer inactivityTimer;
        std::atomic_bool wasClosed;
        std::recursive_mutex closeLock;

        Implementation(
            boost::asio::ip::tcp::socket&& socket, 
            std::string tunnelId, 
            std::weak_ptr<ControlSession> controlSession,
            std::weak_ptr<Service> service
        )
            : socket{std::move(socket)}
            , controlSession{std::move(controlSession)}
            , peekBuffer(4096, '\0')
            , isPublisherSide{false}
            , tunnelId{std::move(tunnelId)}
            , service{std::move(service)}
            , inactivityTimer{socket.get_executor()}
            , wasClosed{false}
            , closeLock{}
        {}
    };
//#####################################################################################################################
    TunnelSession::TunnelSession(
        boost::asio::ip::tcp::socket&& socket, 
        std::string tunnelId, 
        std::weak_ptr<ControlSession> controlSession,
        std::weak_ptr<Service> service
    )
        : impl_{std::make_unique<Implementation>(
            std::move(socket), 
            std::move(tunnelId), 
            std::move(controlSession), 
            std::move(service)
        )}
    {
        spdlog::info("Tunnel side created.");
    }
//---------------------------------------------------------------------------------------------------------------------
    void TunnelSession::resetTimer()
    {
        impl_->inactivityTimer.expires_from_now(boost::posix_time::seconds(InactivityTimeout.count()));
        impl_->inactivityTimer.async_wait([weak = weak_from_this()](auto const& ec) 
        {
            if (ec == boost::asio::error::operation_aborted)
                return;

            auto self = weak.lock();
            if (!self)
                return;

            if (ec)
            {
                // TODO: what to do here?
                //stop();
                return;
            }

            // Check whether the deadline has passed. We compare the deadline against
            // the current time since a new asynchronous operation may have moved the
            // deadline before this actor had a chance to run.
            if (self->impl_->inactivityTimer.expires_at() <= boost::asio::deadline_timer::traits_type::now())
            {
                spdlog::warn("Closing tunnel due to inactivity.");
                self->close();
            }
        });
    }
//---------------------------------------------------------------------------------------------------------------------
    void TunnelSession::peek()
    {
        resetTimer();

        impl_->socket.async_read_some(
            boost::asio::buffer(impl_->peekBuffer, impl_->peekBuffer.size()), 
            [weak = weak_from_this()](const boost::system::error_code& ec, std::size_t bytesTransferred)
            {
                auto self = weak.lock();
                if (!self)
                    return;

                auto service = self->impl_->service.lock();
                if (!service)
                    return;

                if (ec)
                {
                    spdlog::warn("Initial read for tunnel side failed, this will terminate this side of the tunnel.");
                    self->close();
                    return;
                }
                
                auto controlSession = self->impl_->controlSession.lock();
                if (!controlSession)
                {
                    spdlog::warn("Missing control session for new session, this will terminate this tunnel.");
                    self->close();
                    return;
                }

                if (bytesTransferred > 0 && self->impl_->peekBuffer[0] == '{')
                {
                    try 
                    {
                        self->impl_->peekBuffer.resize(bytesTransferred);

                        // {identity: this.identity, tunnelId, serviceId, hiddenPort, publicPort}
                        auto connectionMessage = json::parse(self->impl_->peekBuffer)
                            .get<PublisherConnectionMessage>();
                        self->impl_->peekBuffer.clear();

                        self->impl_->isPublisherSide = true;

                        const auto remoteEndpoint = controlSession->remoteEndpoint();
                        if (self->impl_->socket.remote_endpoint().address() != remoteEndpoint.address())
                        {
                            spdlog::warn("Received tunnel info from another remote than the control socket. This is not allowed.");
                            self->close();
                            return;
                        }

                        service->connectTunnels(
                            connectionMessage.tunnelId,
                            self->impl_->tunnelId
                        );
                        return;
                    }
                    catch(...) 
                    {
                    }
                }

                // assume this is not json from 
                self->impl_->isPublisherSide = false;
                controlSession->informAboutConnection(service->serviceId(), self->impl_->tunnelId);
            }
        );
    }
//---------------------------------------------------------------------------------------------------------------------
    void TunnelSession::link(TunnelSession& other)
    {
        spdlog::info("Connecting tunnel '{}' with '{}'.", impl_->tunnelId, other.impl_->tunnelId);

        // Self -> Other
        if (!impl_->peekBuffer.empty())
        {
            boost::asio::async_write(
                other.impl_->socket,
                boost::asio::buffer(impl_->peekBuffer), 
                [self = shared_from_this(), otherSelf = other.shared_from_this()](auto const& ec, std::size_t)
            {
                if (ec)
                {
                    self->close();
                    otherSelf->close();
                    return;
                }
                self->pipeTo(*otherSelf);
            });
        }
        else
            pipeTo(other);

        // Other -> Self
        if (!other.impl_->peekBuffer.empty())
        {
            boost::asio::async_write(
                impl_->socket,
                boost::asio::buffer(other.impl_->peekBuffer), 
                [self = shared_from_this(), otherSelf = other.shared_from_this()](auto const& ec, std::size_t)
            {
                if (ec)
                {
                    self->close();
                    otherSelf->close();
                    return;
                }
                otherSelf->pipeTo(*self);
            });
        }
        else
            other.pipeTo(*this);
    }
//---------------------------------------------------------------------------------------------------------------------
    void TunnelSession::pipeTo(TunnelSession& other)
    {
        auto doPipe = std::make_shared<std::function<void(
            std::shared_ptr<TunnelSession>, 
            std::shared_ptr<TunnelSession>, 
            std::shared_ptr <std::string>
        )>>();
        *doPipe = [doPipe](
            std::shared_ptr<TunnelSession> self, 
            std::shared_ptr<TunnelSession> other, 
            std::shared_ptr <std::string> buffer
        )
        {
            self->resetTimer();
            self->impl_->socket.async_read_some(
                boost::asio::buffer(*buffer),
                [
                    self = std::move(self), 
                    otherSelf = std::move(other), 
                    buffer = std::move(buffer), 
                    doPipe = std::move(doPipe)
                ]
                (auto const& ec, std::size_t bytesTransferred)
                {
                    if (ec)
                    {
                        self->close();
                        otherSelf->close();
                        return;                   
                    }

                    boost::asio::async_write(
                        otherSelf->impl_->socket,
                        boost::asio::buffer(*buffer, bytesTransferred),
                        [self = std::move(self), otherSelf = std::move(otherSelf), buffer = std::move(buffer), doPipe = std::move(doPipe)]
                        (auto const& ec, std::size_t)
                        {
                            if  (ec)
                            {
                                self->close();
                                otherSelf->close();
                                return;
                            }

                            (*doPipe)(std::move(self), std::move(otherSelf), std::move(buffer));
                        }
                    );
                }
            );
        };
        (*doPipe)(shared_from_this(), other.shared_from_this(), std::make_shared<std::string>(4096, '\0'));
    }
//---------------------------------------------------------------------------------------------------------------------
    void TunnelSession::close()
    {
        std::scoped_lock lock{impl_->closeLock};

        if (impl_->wasClosed)
            return;
        impl_->wasClosed = true;

        spdlog::info("Closing tunnel session '{}'.", impl_->tunnelId);

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
        close();
    }
//---------------------------------------------------------------------------------------------------------------------
    TunnelSession::TunnelSession(TunnelSession&&) = default;
//---------------------------------------------------------------------------------------------------------------------
    TunnelSession& TunnelSession::operator=(TunnelSession&&) = default;
//#####################################################################################################################
}