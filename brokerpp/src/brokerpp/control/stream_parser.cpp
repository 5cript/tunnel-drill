#include <brokerpp/control/stream_parser.hpp>

namespace TunnelBore::Broker
{
    //#####################################################################################################################
    std::optional<json> StreamParser::popMessage()
    {
        const auto position = findCompleteObjectEnd();
        if (position == std::end(m_buffer))
            return std::nullopt;
        return json::parse(consumeObject(position + 1));
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::string::const_iterator StreamParser::findCompleteObjectEnd()
    {
        for (; m_findState.m_offset != m_buffer.size(); ++m_findState.m_offset)
        {
            auto const& c = m_buffer[m_findState.m_offset];

            if (c == '{' && !m_findState.m_insideString)
                ++m_findState.m_curlyCount;
            else if (c == '}' && !m_findState.m_insideString)
            {
                --m_findState.m_curlyCount;
                if (m_findState.m_curlyCount == 0)
                    return std::begin(m_buffer) +
                        static_cast<std::string::const_iterator::difference_type>(m_findState.m_offset);
            }
            else if (c == '"' && !m_findState.m_escape)
                m_findState.m_insideString = !m_findState.m_insideString;
            else if (c == '\\')
                m_findState.m_escape = !m_findState.m_escape;
            if (c != '\\' && m_findState.m_escape)
                m_findState.m_escape = false;
        }
        return std::end(m_buffer);
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::string StreamParser::consumeObject(std::string::const_iterator end)
    {
        auto begin = std::cbegin(m_buffer);
        for (; begin != end && static_cast<bool>(std::isspace(*begin)); ++begin) {}

        std::string result{begin, end};
        m_buffer = std::string{end, std::cend(m_buffer)};
        m_findState = {};
        return result;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void StreamParser::feed(std::string_view text)
    {
        m_buffer.append(text.data(), text.size());
    }
    //#####################################################################################################################
}