
#include <boost/multiprecision/cpp_int.hpp>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

class MemoryUnit
{
  public:
    constexpr MemoryUnit(boost::multiprecision::uint128_t bytes)
        : bytes_(bytes)
    {}
    constexpr MemoryUnit()
        : bytes_(0)
    {}
    template <typename IntegralType>
    constexpr MemoryUnit(IntegralType bytes)
        : bytes_(bytes)
    {}

    friend std::ostream& operator<<(std::ostream& os, const MemoryUnit& mu)
    {
        static const char* suffixes[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
        boost::multiprecision::uint128_t bytes = mu.bytes_;
        int suffix_index = 0;
        while (bytes >= 1024 && suffix_index < 8)
        {
            bytes /= 1024;
            suffix_index++;
        }

        os << std::fixed << std::setprecision(2) << bytes << " " << suffixes[suffix_index];
        return os;
    }

    std::string toString() const
    {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }

    template <typename IntegralType>
    MemoryUnit& operator+=(IntegralType bytes)
    {
        bytes_ += bytes;
        return *this;
    }
    template <typename IntegralType>
    MemoryUnit& operator-=(IntegralType bytes)
    {
        bytes_ -= bytes;
        return *this;
    }
    template <typename IntegralType>
    MemoryUnit& operator*=(IntegralType bytes)
    {
        bytes_ *= bytes;
        return *this;
    }
    template <typename IntegralType>
    MemoryUnit& operator/=(IntegralType bytes)
    {
        bytes_ /= bytes;
        return *this;
    }
    template <typename IntegralType>
    MemoryUnit& operator%=(IntegralType bytes)
    {
        bytes_ %= bytes;
        return *this;
    }
    template <typename IntegralType>
    MemoryUnit& operator&=(IntegralType bytes)
    {
        bytes_ &= bytes;
        return *this;
    }
    template <typename IntegralType>
    MemoryUnit& operator|=(IntegralType bytes)
    {
        bytes_ |= bytes;
        return *this;
    }
    template <typename IntegralType>
    MemoryUnit& operator^=(IntegralType bytes)
    {
        bytes_ ^= bytes;
        return *this;
    }
    template <typename IntegralType>
    MemoryUnit& operator<<=(IntegralType bytes)
    {
        bytes_ <<= bytes;
        return *this;
    }
    template <typename IntegralType>
    MemoryUnit& operator>>=(IntegralType bytes)
    {
        bytes_ >>= bytes;
        return *this;
    }
    template <typename IntegralType>
    MemoryUnit& operator=(IntegralType bytes)
    {
        bytes_ >>= bytes;
        return *this;
    }

  private:
    boost::multiprecision::uint128_t bytes_;
};

constexpr MemoryUnit operator"" _B(unsigned long long bytes)
{
    return MemoryUnit(bytes);
}

constexpr MemoryUnit operator"" _KB(unsigned long long kilobytes)
{
    return MemoryUnit(kilobytes * 1024);
}

constexpr MemoryUnit operator"" _MB(unsigned long long megabytes)
{
    return MemoryUnit(megabytes * 1024 * 1024);
}

constexpr MemoryUnit operator"" _GB(unsigned long long gigabytes)
{
    return MemoryUnit(gigabytes * 1024 * 1024 * 1024);
}

constexpr MemoryUnit operator"" _TB(unsigned long long terabytes)
{
    return MemoryUnit(terabytes * 1024 * 1024 * 1024 * 1024);
}

constexpr MemoryUnit operator"" _PB(unsigned long long petabytes)
{
    return MemoryUnit(petabytes * 1024 * 1024 * 1024 * 1024 * 1024);
}

constexpr MemoryUnit operator"" _EB(unsigned long long exabytes)
{
    return MemoryUnit(exabytes * 1024 * 1024 * 1024 * 1024 * 1024 * 1024);
}