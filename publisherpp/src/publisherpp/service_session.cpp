#include <publisherpp/service_session.hpp>

#include <sharedpp/pipe_operation.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <mutex>

namespace TunnelBore::Publisher
{
    struct ServiceSession::Implementation
    {
        Implementation(boost::asio::ip::tcp::socket&& socket)
            : socket{std::move(socket)}
            , onClose{}
            , active{true}
            , inactivityTimer{this->socket.get_executor()}
            , remoteAddress{[this]{
                return this->socket.remote_endpoint().address().to_string() + ":" +
                    std::to_string(this->socket.remote_endpoint().port());
            }()}
        {}
        boost::asio::ip::tcp::socket socket;
        std::function<void()> onClose;
        std::recursive_mutex mutex;
        bool active;
        boost::asio::deadline_timer inactivityTimer;
        std::string remoteAddress;
    };
    ServiceSession::ServiceSession(boost::asio::ip::tcp::socket&& socket)
        : impl_{std::make_unique<Implementation>(std::move(socket))}
    {}
    ServiceSession::~ServiceSession()
    {
        close();
    }
    ServiceSession::ServiceSession(ServiceSession&&) = default;
    ServiceSession& ServiceSession::operator=(ServiceSession&&) = default;

    void ServiceSession::setOnClose(std::function<void()> onClose)
    {
        impl_->onClose = std::move(onClose);
    }
    void ServiceSession::close()
    {
        std::scoped_lock lock{impl_->mutex};
        boost::system::error_code ec;
        impl_->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        if (ec && ec != boost::asio::error::not_connected)
        {
            spdlog::error("ServiceSession::close: socket.shutdown() failed: {}", std::string{ec.message()});
        }
        impl_->active = false;
        impl_->onClose();
    }
    bool ServiceSession::active()
    {
        std::scoped_lock lock{impl_->mutex};
        return impl_->active;
    }
    void ServiceSession::resetTimer()
    {
        std::scoped_lock lock{impl_->mutex};
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
        std::scoped_lock lock{impl_->mutex};
        return impl_->socket;
    }
    std::string ServiceSession::remoteAddress()
    {
        return impl_->remoteAddress;
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