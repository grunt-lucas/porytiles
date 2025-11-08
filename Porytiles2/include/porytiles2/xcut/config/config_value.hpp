#pragma once

#include <string>
#include <utility>
#include <vector>

#include "porytiles2/utilities/text/text_formatter.hpp"

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
     * @param source_details A vector of strings with the optional source details
     */
    ConfigValue(T value, std::string name, std::string source, const std::vector<std::string> &source_details)
        : value_{std::move(value)}, name_{std::move(name)}, source_{std::move(source)}, source_details_{source_details}
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

    [[nodiscard]] std::pair<std::vector<std::string>, std::vector<std::vector<FormatParam>>> format_data() const
    {
        // foo = 3
        // Source: ./porytiles.yaml:12
        // ... details here

        std::vector<std::string> err_text{};
        std::vector<std::vector<FormatParam>> params{};

        err_text.emplace_back("{} = {}");
        params.push_back(std::vector{FormatParam{name(), Style::bold}, FormatParam{value(), Style::bold}});
        err_text.emplace_back("Source: {}");
        params.push_back(std::vector{FormatParam{source(), Style::bold}});

        // Add source details if available
        if (!source_details().empty()) {
            err_text.emplace_back("");
            params.emplace_back();
            std::ranges::copy(source_details(), std::back_inserter(err_text));
        }

        return {err_text, params};
    }

  private:
    T value_;
    std::string name_;
    std::string source_;
    std::vector<std::string> source_details_;
};

} // namespace porytiles2
