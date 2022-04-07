#include <brokerpp/controller.hpp>
#include <brokerpp/control/control_session.hpp>
#include <spdlog/spdlog.h>

namespace TunnelBore::Broker
{

Controller::Controller(boost::asio::io_context& context, std::function <void(boost::system::error_code)> on_error)
    : ws_{&context, std::move(on_error)}
{
}
Controller::~Controller()
{
    ws_.stop();
}

void Controller::start(unsigned short port)
{
    ws_.start([weak = weak_from_this()](std::shared_ptr<attender::websocket::connection> connection){
        auto shared = weak.lock();
        if (!shared)
        {
            spdlog::error("Connection attempt received after server is dead.");
            return;
        }        

        std::scoped_lock lock{shared->guard_};
        spdlog::info("New controller connection.");

        const auto id = shared->generator_.generate_id();
        connection->create_session<ControlSession>(id).setup();
        shared->connections_[id] = std::move(connection);
    }, std::to_string(port));
}
void Controller::stop()
{
    ws_.stop();
}

}