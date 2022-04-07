#pragma once

#include <boost/leaf.hpp>

#include <memory>

namespace boost::asio
{
  class io_context;
}
namespace boost::asio::ip
{
  class tcp;
  template <typename T>
  class basic_endpoint;
}

namespace TunnelBore::Broker
{

class ConnectionAcceptor : public std::enable_shared_from_this<ConnectionAcceptor>
{
public:
  ConnectionAcceptor(boost::asio::io_context& context);
  ~ConnectionAcceptor();
  ConnectionAcceptor(ConnectionAcceptor const&) = delete;
  ConnectionAcceptor(ConnectionAcceptor&&);
  ConnectionAcceptor& operator=(ConnectionAcceptor const&) = delete;
  ConnectionAcceptor& operator=(ConnectionAcceptor&&);

  boost::leaf::result<void> start(boost::asio::ip::basic_endpoint<boost::asio::ip::tcp> const& bindEndpoint);
  void stop();

private:
  void acceptOnce();

private:
  struct Implementation;
  std::unique_ptr <Implementation> impl_;
};

}