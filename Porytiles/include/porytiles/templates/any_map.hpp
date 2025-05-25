#pragma once

#include <any>
#include <optional>
#include <typeindex>
#include <unordered_map>

#include "../panic/panic.hpp"

namespace porytiles {

class AnyMap {
  public:
    AnyMap() = default;

    template <typename T> [[nodiscard]] std::optional<T> Try(const std::string &key) const {
        if (!Contains(key)) {
            return std::nullopt;
        }
        try {
            return std::optional{std::any_cast<T>(config_.at(key))};
        } catch (const std::bad_any_cast &) {
            return std::nullopt;
        }
    }

    template <typename T> [[nodiscard]] std::optional<T> Get(const std::string &key) const {
        if (!Contains(key)) {
            Panic("Key not found: " + key);
        }
        try {
            return std::optional{std::any_cast<T>(config_.at(key))};
        } catch (const std::bad_any_cast &) {
            Panic("Invalid type requested for key: " + key);
        }
    }

    [[nodiscard]] std::any GetAny(const std::string &key) const {
        if (!Contains(key)) {
            Panic("Key not found: " + key);
        }
        return config_.at(key);
    }

    void Put(const std::string &key, const std::any &value) {
        config_.insert_or_assign(key, value);
    }

    [[nodiscard]] bool Contains(const std::string &key) const {
        return config_.contains(key);
    }

    [[nodiscard]] std::optional<std::type_index> GetType(const std::string &key) const {
        if (!Contains(key)) {
            return std::nullopt;
        }
        return config_.at(key).type();
    }

  private:
    std::unordered_map<std::string, std::any> config_;
};

} // namespace porytiles
