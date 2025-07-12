#pragma once

#include <any>
#include <expected>
#include <optional>
#include <typeindex>
#include <unordered_map>

#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

class ArtifactBundle {
  public:
    ArtifactBundle() = default;

    // -- Range-for support --
    using iterator = std::unordered_map<std::string, std::any>::iterator;
    using const_iterator = std::unordered_map<std::string, std::any>::const_iterator;
    iterator begin() noexcept {
        return config_.begin();
    }
    iterator end() noexcept {
        return config_.end();
    }
    [[nodiscard]] const_iterator begin() const noexcept {
        return config_.begin();
    }
    [[nodiscard]] const_iterator end() const noexcept {
        return config_.end();
    }
    [[nodiscard]] const_iterator cbegin() const noexcept {
        return config_.cbegin();
    }
    [[nodiscard]] const_iterator cend() const noexcept {
        return config_.cend();
    }
    // -- Range-for support --

    [[nodiscard]] std::optional<std::any> get(const std::string &key) const {
        if (!contains(key)) {
            return std::nullopt;
        }
        return std::optional{config_.at(key)};
    }

    template <typename T>
    [[nodiscard]] std::optional<T> get_unwrap(const std::string &key) const {
        if (!contains(key)) {
            return std::nullopt;
        }
        try {
            return std::optional{std::any_cast<T>(config_.at(key))};
        } catch (const std::bad_any_cast &) {
            panic("invalid type requested for key: " + key);
        }
    }

    void put(const std::string &key, const std::any &value) {
        config_.insert_or_assign(key, value);
    }

    [[nodiscard]] bool contains(const std::string &key) const {
        return config_.contains(key);
    }

    [[nodiscard]] std::optional<std::type_index> type_index_of(const std::string &key) const {
        if (!contains(key)) {
            return std::nullopt;
        }
        return config_.at(key).type();
    }

  private:
    std::unordered_map<std::string, std::any> config_;
};

} // namespace porytiles2
