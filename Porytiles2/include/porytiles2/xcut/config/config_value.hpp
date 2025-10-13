#pragma once

#include <string>
#include <utility>

namespace porytiles2 {

/**
 * @brief A container that wraps a configuration value with its source information.
 *
 * @details
 * ConfigValue provides transparent access to configuration values while also tracking where each value originated
 * (e.g., which config provider supplied it). It supports implicit conversion to the underlying type for ergonomic
 * usage.
 *
 * @tparam T The type of the configuration value
 */
template <typename T>
class ConfigValue {
  public:
    /**
     * @brief Constructs a ConfigValue with a value and its source information.
     *
     * @param value The configuration value
     * @param source A string describing where this value came from
     */
    ConfigValue(T value, std::string source) : value_{std::move(value)}, source_{std::move(source)} {}

    /**
     * @brief Implicit conversion to const reference of the underlying value.
     *
     * @details
     * Allows ConfigValue to be used transparently where the underlying type is expected.
     *
     * @return A const reference to the stored value
     */
    operator const T &() const &
    {
        return value_;
    }

    /**
     * @brief Implicit conversion to rvalue reference of the underlying value.
     *
     * @details
     * Enables move semantics when the ConfigValue is an rvalue.
     *
     * @return An rvalue reference to the stored value
     */
    operator T &&() &&
    {
        return std::move(value_);
    }

    /**
     * @brief Gets a const reference to the underlying value.
     *
     * @return A const reference to the stored value
     */
    [[nodiscard]] const T &value() const &
    {
        return value_;
    }

    /**
     * @brief Gets an rvalue reference to the underlying value.
     *
     * @return An rvalue reference to the stored value
     */
    [[nodiscard]] T &&value() &&
    {
        return std::move(value_);
    }

    /**
     * @brief Gets the source information for this configuration value.
     *
     * @details
     * The source string describes where this value originated, such as:
     * - "DefaultProvider: default value"
     * - "MockTomlProvider: from toml file"
     * - "computed: num_tiles_total (Provider: X) - num_tiles_primary (Provider: Y)"
     *
     * @return A const reference to the source string
     */
    [[nodiscard]] const std::string &source() const
    {
        return source_;
    }

  private:
    T value_;
    std::string source_;
};

} // namespace porytiles2
