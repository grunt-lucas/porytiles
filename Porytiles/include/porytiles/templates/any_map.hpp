#pragma once

#include <any>
#include <optional>
#include <unordered_map>

#include "../panic/panic.hpp"

namespace porytiles {

class AnyMap {
  public:
    AnyMap() = default;

    template <typename T> [[nodiscard]] std::optional<T> Try(const std::string &key) {
        if (!config_.contains(key)) {
            return std::nullopt;
        }
        try {
            return std::optional{std::any_cast<T>(config_[key])};
        } catch (const std::bad_any_cast &) {
            return std::nullopt;
        }
    }

    template <typename T> [[nodiscard]] std::optional<T> Get(const std::string &key) {
        if (!config_.contains(key)) {
            Panic("Key not found: " + key);
        }
        try {
            return std::optional{std::any_cast<T>(config_[key])};
        } catch (const std::bad_any_cast &) {
            Panic("Invalid type requested for key: " + key);
        }
    }

    void Put(const std::string &key, const std::any &value) {
        config_.insert_or_assign(key, value);
    }

    [[nodiscard]] bool Contains(const std::string &key) const {
        return config_.contains(key);
    }

  private:
    std::unordered_map<std::string, std::any> config_;
};

} // namespace porytiles
