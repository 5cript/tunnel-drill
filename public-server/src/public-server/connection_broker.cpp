#include <public-server/connection_broker.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <mutex>
#include <shared_mutex>

namespace leaf = boost::leaf;

namespace TunnelBore::PublicServer
{
//#####################################################################################################################
struct ConnectionBroker::Implementation
{
  boost::asio::ip::tcp::acceptor acceptor_;
  std::shared_mutex acceptorStopGuard_;

  Implementation(boost::asio::io_context& context)
    : acceptor_{context}
    , acceptorStopGuard_{}
  {}
};
//#####################################################################################################################
ConnectionBroker::ConnectionBroker(boost::asio::io_context& context)
  : impl_{std::make_unique<Implementation>(context)}
{
}
//---------------------------------------------------------------------------------------------------------------------
ConnectionBroker::~ConnectionBroker() = default;
//---------------------------------------------------------------------------------------------------------------------
ConnectionBroker::ConnectionBroker(ConnectionBroker&&) = default;
//---------------------------------------------------------------------------------------------------------------------
ConnectionBroker& ConnectionBroker::operator=(ConnectionBroker&&) = default;
//---------------------------------------------------------------------------------------------------------------------
boost::leaf::result<void> ConnectionBroker::start(boost::asio::ip::tcp::endpoint const& bindEndpoint)
{
  stop();
  boost::system::error_code ec;

  impl_->acceptor_.open(bindEndpoint.protocol(), ec);
  if (ec)
    return leaf::new_error("Could not open http server acceptor.", ec);

  impl_->acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec)
    return leaf::new_error("Could not configure socket to reuse address.", ec);

  impl_->acceptor_.bind(bindEndpoint, ec);
  if (ec)
    return leaf::new_error("Could not bind socket.", ec);

  impl_->acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec)
    return leaf::new_error("Could not listen on socket.", ec);

  acceptOnce();
  return {};
}
//---------------------------------------------------------------------------------------------------------------------
void ConnectionBroker::acceptOnce()
{
  std::shared_ptr<boost::asio::ip::tcp::socket> socket =
    std::make_shared<boost::asio::ip::tcp::socket>(impl_->acceptor_.get_executor());
  impl_->acceptor_.async_accept(
    *socket,
    [self = shared_from_this(), socket](
      boost::system::error_code ec) mutable {
      std::shared_lock lock{self->impl_->acceptorStopGuard_};

      if (ec == boost::asio::error::operation_aborted)
        return;

      if (!self->impl_->acceptor_.is_open())
        return;

      if (!ec)
      {
        // TODO: on accept
        //acceptCallback(std::move(socket));
      }

      self->acceptOnce();
    });
}
//---------------------------------------------------------------------------------------------------------------------
void ConnectionBroker::stop()
{
  std::unique_lock lock{impl_->acceptorStopGuard_};
  impl_->acceptor_.close();
}
//#####################################################################################################################
}