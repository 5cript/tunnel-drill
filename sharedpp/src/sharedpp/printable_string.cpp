#include <sharedpp/printable_string.hpp>

#include <algorithm>
#include <cctype>

std::string makePrintableString(std::string_view input)
{
    std::string result;
    result.reserve(input.size());
    std::for_each(input.begin(), input.end(), [&result](auto c) {
        if (std::isprint(c) && !std::isspace(c))
            result.push_back(c);
        else
        {
            constexpr auto hexDigits = "0123456789ABCDEF";

            result.push_back('\\');
            result.push_back('x');
            result.push_back(hexDigits[c >> 4]);
            result.push_back(hexDigits[c & 0xF]);
        }
    });
    return result;
}