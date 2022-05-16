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
            std::vector<Publisher> publishers;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_EX(Publisher, identity, pass, salt, maxServices)
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_EX(UsersFile, pepper, publishers)
    }
    //#####################################################################################################################
    User::User(std::string identity)
        : identity_(std::move(identity))
    {}
    //---------------------------------------------------------------------------------------------------------------------
    std::string User::identity() const
    {
        return identity_;
    }
    //#####################################################################################################################
    struct UserControl::Implementation
    {
        UsersFile users;
    };
    //#####################################################################################################################
    UserControl::UserControl()
        : impl_{std::make_unique<Implementation>()}
    {
        impl_->users = json::parse(loadHomeFile("users.json")).get<UsersFile>();
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::optional<User> UserControl::getUser(std::string const& name, std::string const& password)
    {
        for (auto const& user : impl_->users.publishers)
        {
            if (user.identity == name)
            {
                auto hash = attender::sha512(std::string{password} + "_" + user.salt + "_" + impl_->users.pepper);
                if (!hash)
                    return std::nullopt;

                if (*hash == user.pass)
                    return User{name};

                return std::nullopt;
            }
        }
        return std::nullopt;
    }
    //---------------------------------------------------------------------------------------------------------------------
    UserControl::~UserControl() = default;
    //---------------------------------------------------------------------------------------------------------------------
    UserControl::UserControl(UserControl&&) = default;
    //---------------------------------------------------------------------------------------------------------------------
    UserControl& UserControl::operator=(UserControl&&) = default;
    //#####################################################################################################################
}