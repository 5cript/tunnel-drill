#include <brokerpp/publisher/service.hpp>
#include <brokerpp/session.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <mutex>
#include <shared_mutex>

namespace leaf = boost::leaf;

namespace TunnelBore::Broker
{
//#####################################################################################################################
struct Service::Implementation
{
  boost::asio::ip::tcp::acceptor acceptor;
  std::shared_mutex acceptorStopGuard;
  std::vector<std::shared_ptr<Session>> sessions;
  ServiceInfo info;
  boost::asio::ip::tcp::endpoint bindEndpoint;

  Implementation(
      boost::asio::io_context& context, 
      ServiceInfo const& info,
      boost::asio::ip::tcp::endpoint bindEndpoint
    )
    : acceptor{context}
    , acceptorStopGuard{}
    , sessions{}
    , info{info}
    , bindEndpoint{bindEndpoint}
  {}
};
//#####################################################################################################################
Service::Service(
    boost::asio::io_context& context, 
    ServiceInfo const& info, 
    boost::asio::ip::basic_endpoint<boost::asio::ip::tcp> const& bindEndpoint
)
  : impl_{std::make_unique<Implementation>(context, info, bindEndpoint)}
{
}
//---------------------------------------------------------------------------------------------------------------------
Service::~Service()
{
  stop();
}
//---------------------------------------------------------------------------------------------------------------------
Service::Service(Service&&) = default;
//---------------------------------------------------------------------------------------------------------------------
Service& Service::operator=(Service&&) = default;
//---------------------------------------------------------------------------------------------------------------------
boost::leaf::result<void> Service::start()
{
    stop();
    boost::system::error_code ec;

    impl_->acceptor.open(impl_->bindEndpoint.protocol(), ec);
    if (ec)
        return leaf::new_error("Could not open http server acceptor.", ec);

    impl_->acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec)
        return leaf::new_error("Could not configure socket to reuse address.", ec);

    impl_->acceptor.bind(impl_->bindEndpoint, ec);
    if (ec)
        return leaf::new_error("Could not bind socket.", ec);

    impl_->acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec)
        return leaf::new_error("Could not listen on socket.", ec);

    acceptOnce();
    return {};
}
//---------------------------------------------------------------------------------------------------------------------
void Service::acceptOnce()
{
  std::shared_ptr<boost::asio::ip::tcp::socket> socket =
    std::make_shared<boost::asio::ip::tcp::socket>(impl_->acceptor.get_executor());
  impl_->acceptor.async_accept(
    *socket,
    [self = shared_from_this(), socket](boost::system::error_code ec) mutable {
      std::shared_lock lock{self->impl_->acceptorStopGuard};

      if (ec == boost::asio::error::operation_aborted)
        return;

      if (!self->impl_->acceptor.is_open())
        return;

      if (!ec)
        self->impl_->sessions.push_back(std::make_shared<Session>(std::move(*socket)));

      self->acceptOnce();
    });
}
//---------------------------------------------------------------------------------------------------------------------
void Service::stop()
{
  std::unique_lock lock{impl_->acceptorStopGuard};
  impl_->acceptor.close();
}
//#####################################################################################################################
}