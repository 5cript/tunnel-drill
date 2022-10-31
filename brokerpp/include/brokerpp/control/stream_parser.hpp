#pragma once

#include <sharedpp/json.hpp>

#include <string>
#include <optional>

namespace TunnelBore::Broker
{
    class StreamParser
    {
      public:
        void feed(std::string_view txt);
        std::optional<json> popMessage();

      private:
        std::string::const_iterator findCompleteObjectEnd();
        std::string consumeObject(std::string::const_iterator end);

      private:
        std::string m_buffer;
        struct FindMemoizer
        {
            int m_curlyCount = 0;
            bool m_escape = false;
            bool m_insideString = false;
            std::size_t m_offset = 0;
        } m_findState;
    };
}