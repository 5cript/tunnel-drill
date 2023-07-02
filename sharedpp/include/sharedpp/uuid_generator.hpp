#include <string>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

class uuid_generator
{
  public:
    std::string generate_id() const
    {
        return boost::uuids::to_string(gen_());
    }

  private:
    mutable boost::uuids::random_generator gen_;
};