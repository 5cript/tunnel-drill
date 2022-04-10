#include <brokerpp/publisher/tunnel_session.hpp>
#include <brokerpp/publisher/service.hpp>
#include <brokerpp/control/control_session.hpp>
#include <brokerpp/json.hpp>

#include <spdlog/spdlog.h>

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
        peek();
    }
//---------------------------------------------------------------------------------------------------------------------
    void TunnelSession::peek()
    {
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
                    service->closeTunnelSide(self->impl_->tunnelId);
                    return;
                }

                if (bytesTransferred > 0 && self->impl_->peekBuffer[0] == '{')
                {
                    try 
                    {
                        std::cout << self->impl_->peekBuffer << "\n";

                        // {identity: this.identity, tunnelId, serviceId, hiddenPort, publicPort}
                        // TODO:
                        auto connectionMessage = json::parse(self->impl_->peekBuffer)
                            .get<PublisherConnectionMessage>();
                        self->impl_->isPublisherSide = true;

                        service->connectTunnels(
                            connectionMessage.tunnelId,
                            self->impl_->tunnelId
                        );
                    }
                    catch(...) 
                    {
                        // assume this is not json from 
                        self->impl_->isPublisherSide = false;

                        auto controlSession = self->impl_->controlSession.lock();
                        if (!controlSession)
                        {
                            spdlog::warn("Missing control session for new session, this will terminate this tunnel.");
                            service->closeTunnelSide(self->impl_->tunnelId);
                            return;
                        }

                        controlSession->informAboutConnection(service->serviceId(), self->impl_->tunnelId);
                    }
                }
            }
        );
    }
//---------------------------------------------------------------------------------------------------------------------
    void TunnelSession::link(TunnelSession& other)
    {
        // TODO:
    }
//---------------------------------------------------------------------------------------------------------------------
    void TunnelSession::close()
    {
        boost::system::error_code ignore;
        impl_->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
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