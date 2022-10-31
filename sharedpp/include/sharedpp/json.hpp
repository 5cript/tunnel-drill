#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <memory>

using json = nlohmann::json;

namespace nlohmann
{
    template <class T>
    void to_json(nlohmann::json& j, const std::optional<T>& v)
    {
        if (v.has_value())
            j = *v;
        else
            j = nullptr;
    }

    template <class T>
    void from_json(const nlohmann::json& j, std::optional<T>& v)
    {
        if (j.is_null())
            v = std::nullopt;
        else
            v = j.get<T>();
    }

    template <class T>
    void to_json(nlohmann::json& j, const std::shared_ptr<T>& v)
    {
        if (v)
            j = *v;
        else
            j = nullptr;
    }

    template <class T>
    void from_json(const nlohmann::json& j, std::shared_ptr<T>& v)
    {
        v = std::make_shared<T>();
        for (auto& e : j)
            v->push_back(e.get<T>());
    }
} // namespace nlohmann

#define NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_EX(Type, ...) \
    [[maybe_unused]] inline void to_json(nlohmann::json& nlohmann_json_j, const Type& nlohmann_json_t) \
    { \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO, __VA_ARGS__)) \
    } \
    [[maybe_unused]] inline void from_json(const nlohmann::json& nlohmann_json_j, Type& nlohmann_json_t) \
    { \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM, __VA_ARGS__)) \
    }