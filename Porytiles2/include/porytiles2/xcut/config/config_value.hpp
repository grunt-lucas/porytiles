#pragma once

#include <string>
#include <utility>
#include <vector>

namespace porytiles2 {

/**
 * @brief A container that wraps a configuration value with its name and source information.
 *
 * @details
 * ConfigValue provides transparent access to configuration values while also tracking the config value's name and where
 * each value originated (e.g., which ConfigProvider supplied it). It supports implicit conversion to the underlying
 * type for ergonomic usage.
 *
 * @tparam T The type of the configuration value
 */
template <typename T>
class ConfigValue {
  public:
    /**
     * @brief Constructs a ConfigValue with a value, name, and source information.
     *
     * @param value The configuration value
     * @param name The name of the configuration value (e.g., "num_tiles_primary")
     * @param source A string describing where this value came from
     */
    ConfigValue(T value, std::string name, std::string source)
        : value_{std::move(value)}, name_{std::move(name)}, source_{std::move(source)}
    {
    }

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
     * @brief Gets the name of this configuration value.
     *
     * @details
     * The name identifies the configuration value, such as:
     * - "num_tiles_primary"
     * - "num_tiles_secondary"
     * - "incremental_build_mode"
     *
     * @return A const reference to the name string
     */
    [[nodiscard]] const std::string &name() const
    {
        return name_;
    }

    /**
     * @brief Gets the source information for this configuration value.
     *
     * @details
     * The source string describes where this value originated, such as:
     * - "default value"
     * - "./porytiles.yaml:12"
     * - "$PORYTILES_FIELDMAP_NUM_TILES_PRIMARY"
     *
     * @return A const reference to the source string
     */
    [[nodiscard]] const std::string &source() const
    {
        return source_;
    }

    /**
     * @brief Gets the source details for this configuration value.
     *
     * @details
     * The source details supplement the source string with additional context. For example, the YAML file provider may
     * use the details string to supplement the file name and line number with a contextual view of the YAML file.
     *
     * @return A const reference to the source details vector
     */
    [[nodiscard]] const std::vector<std::string> &source_details() const
    {
        return source_details_;
    }

  private:
    T value_;
    std::string name_;
    std::string source_;
    std::vector<std::string> source_details_;
};

} // namespace porytiles2
