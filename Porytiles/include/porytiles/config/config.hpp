#pragma once

#include <any>
#include <optional>
#include <unordered_map>

#include "../panic/panic.hpp"

namespace porytiles {

class Config {
    std::unordered_map<std::string, std::any> config_;

    template <typename T> T get(const std::string &key) {
        try {
            return std::any_cast<T>(config_[key]);
        } catch (const std::bad_any_cast &) {
            Panic("Invalid type requested for key: " + key);
        }
    }

  public:
    Config() = default;

    template <typename T> std::optional<T> Try(const std::string &key) {
        if (!config_.contains(key)) {
            return std::nullopt;
        }
        return std::optional{get<T>(key)};
    }

    template <typename T> std::optional<T> Get(const std::string &key) {
        if (!config_.contains(key)) {
            Panic("Key not found: " + key);
        }
        return std::optional{get<T>(key)};
    }

    void Put(const std::string &key, const std::any &value) {
        config_.insert_or_assign(key, value);
    }
};

} // namespace porytiles
