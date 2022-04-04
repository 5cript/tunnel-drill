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

namespace TunnelBore::PublicServer
{

class ConnectionBroker : public std::enable_shared_from_this<ConnectionBroker>
{
public:
  ConnectionBroker(boost::asio::io_context& context);
  ~ConnectionBroker();
  ConnectionBroker(ConnectionBroker const&) = delete;
  ConnectionBroker(ConnectionBroker&&);
  ConnectionBroker& operator=(ConnectionBroker const&) = delete;
  ConnectionBroker& operator=(ConnectionBroker&&);

  boost::leaf::result<void> start(boost::asio::ip::basic_endpoint<boost::asio::ip::tcp> const& bindEndpoint);
  void stop();

private:
  void acceptOnce();

private:
  struct Implementation;
  std::unique_ptr <Implementation> impl_;
};

}