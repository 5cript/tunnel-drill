#pragma once

#include <attender/session/basic_authorizer.hpp>

#include <memory>

namespace TunnelBore::Broker
{
    class UserControl
    {
    public:
        class SingleUseAuthorizer : public attender::basic_authorizer
        {
        public:
            SingleUseAuthorizer(UserControl* userControl);

            bool accept_authentication(std::string_view user, std::string_view password) override;
            std::string realm() const override;

        public:
            std::string identity() const;

        private:
            UserControl* userControl_;
            std::string identity_;
        };

        UserControl();
        ~UserControl();
        UserControl(UserControl const&) = delete;
        UserControl(UserControl&&);
        UserControl& operator=(UserControl const&) = delete;
        UserControl& operator=(UserControl&&);

        SingleUseAuthorizer authenticateOnce();

    private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}