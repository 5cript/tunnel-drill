#include <publisherpp/service_session.hpp>

#include <sharedpp/pipe_operation.hpp>
#include <boost/asio/deadline_timer.hpp>

namespace TunnelBore::Publisher
{
    struct ServiceSession::Implementation
    {
        Implementation(boost::asio::ip::tcp::socket&& socket)
            : socket{std::move(socket)}
            , onClose{}
            , active{true}
            , inactivityTimer{this->socket.get_executor()}
        {}
        boost::asio::ip::tcp::socket socket;
        std::function<void()> onClose;
        bool active;
        boost::asio::deadline_timer inactivityTimer;
    };
    ServiceSession::ServiceSession(boost::asio::ip::tcp::socket&& socket)
        : impl_{std::make_unique<Implementation>(std::move(socket))}
    {}
    ServiceSession::~ServiceSession() = default;
    ServiceSession::ServiceSession(ServiceSession&&) = default;
    ServiceSession& ServiceSession::operator=(ServiceSession&&) = default;

    void ServiceSession::setOnClose(std::function<void()> onClose)
    {
        impl_->onClose = std::move(onClose);
    }
    void ServiceSession::close()
    {
        impl_->socket.close();
        impl_->active = false;
        impl_->onClose();
    }
    bool ServiceSession::active()
    {
        return impl_->active;
    }
    void ServiceSession::resetTimer()
    {
        impl_->inactivityTimer.expires_from_now(boost::posix_time::seconds(30));
        impl_->inactivityTimer.async_wait([weak = weak_from_this()](boost::system::error_code const& error) {
            if (error)
                return;
            auto session = weak.lock();
            if (!session)
                return;
            session->close();
        });
    }
    boost::asio::ip::tcp::socket& ServiceSession::socket()
    {
        return impl_->socket;
    }
    std::string ServiceSession::remoteAddress()
    {
        return impl_->socket.remote_endpoint().address().to_string() + ":" +
            std::to_string(impl_->socket.remote_endpoint().port());
    }
    void ServiceSession::pipeTo(ServiceSession& other)
    {
        resetTimer();

        auto pipeOperation =
            std::make_shared<PipeOperation<ServiceSession>>(shared_from_this(), other.shared_from_this());
        // pipeOperation self-holds
        pipeOperation->doPipe();
    }
}