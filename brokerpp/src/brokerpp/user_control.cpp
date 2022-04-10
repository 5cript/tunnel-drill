#include <brokerpp/user_control.hpp>
#include <brokerpp/load_home_file.hpp>
#include <brokerpp/json.hpp>

#include <attender/utility/sha.hpp>

namespace TunnelBore::Broker
{
    namespace
    {
        struct Publisher
        {
            std::string identity;
            std::string pass; // sha512(sha512(actual) + "_" + salt + "_" + pepper)
            std::string salt;
            int maxServices;
        };
        struct UsersFile
        {
            std::string pepper;
            std::vector <Publisher> publishers;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Publisher, identity, pass, salt, maxServices)
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UsersFile, pepper, publishers)
    }
    //#####################################################################################################################
    struct UserControl::Implementation
    {
        UsersFile users;
    };
    //#####################################################################################################################
    bool UserControl::SingleUseAuthorizer::accept_authentication(std::string_view name, std::string_view password)
    {
        for (auto const& user : userControl_->impl_->users.publishers)
        {
            if (user.identity == name)
            {
                auto hash = attender::sha512(std::string{password} + "_" + user.salt + "_" + userControl_->impl_->users.pepper);
                if (!hash)
                    return false;

                if (*hash == user.pass)
                {
                    identity_ = name;
                    return true;
                }

                return false;
            }
        }
        return false;
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::string UserControl::SingleUseAuthorizer::realm() const
    {
        return "tunnelBore";
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::string UserControl::SingleUseAuthorizer::identity() const
    {
        return identity_;
    }
    //---------------------------------------------------------------------------------------------------------------------
    UserControl::SingleUseAuthorizer::SingleUseAuthorizer(UserControl* userControl)
        : userControl_{userControl}
    {
    }
    //#####################################################################################################################
    UserControl::UserControl()
        : impl_{std::make_unique<Implementation>()}
    {
        impl_->users = json::parse(loadHomeFile("users.json")).get<UsersFile>();
    }
    //---------------------------------------------------------------------------------------------------------------------
    UserControl::~UserControl() = default;
    //---------------------------------------------------------------------------------------------------------------------
    UserControl::UserControl(UserControl&&) = default;
    //---------------------------------------------------------------------------------------------------------------------
    UserControl& UserControl::operator=(UserControl&&) = default;
    //---------------------------------------------------------------------------------------------------------------------
    UserControl::SingleUseAuthorizer UserControl::authenticateOnce()
    {
        return SingleUseAuthorizer{this};
    }
    //#####################################################################################################################
}