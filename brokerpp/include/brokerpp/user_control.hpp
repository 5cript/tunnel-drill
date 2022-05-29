#pragma once

#include <memory>
#include <optional>
#include <string>

namespace TunnelBore::Broker
{
    class User
    {
        friend class UserControl;

      private:
        User(std::string email, std::string identity);

      public:
        std::string identity() const;

      private:
        std::string email_;
        std::string identity_;
    };

    class UserControl
    {
      public:
        UserControl();
        ~UserControl();
        UserControl(UserControl const&) = delete;
        UserControl(UserControl&&);
        UserControl& operator=(UserControl const&) = delete;
        UserControl& operator=(UserControl&&);

        std::optional<User> getUser(std::string const& name, std::string const& password);

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}