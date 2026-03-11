#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace porytiles2 {

/**
 * @brief A lightweight wrapper for per-field configuration overrides with source metadata.
 *
 * @details
 * ConfigOverride replaces @c std::optional<T> in configuration structs where individual override fields need to carry
 * provider-specific source metadata (source_key, canonical_name). This metadata is set at parse time by the provider
 * (e.g., YAML parser, CLI parser) and later used by @c ConfigValue::derive() to construct a child @c ConfigValue<T>
 * with proper provenance.
 *
 * Implicit constructors from @c T and @c std::nullopt_t preserve ergonomic usage in tests and initializer lists.
 *
 * @tparam T The type of the override value
 */
template <typename T>
struct ConfigOverride {
    std::optional<T> value{std::nullopt};
    std::string source_key;
    std::string canonical_name;
    std::string source_info;
    std::vector<std::string> source_details;

    ConfigOverride() = default;

    /**
     * @brief Constructs a ConfigOverride with a value and no source metadata.
     *
     * @details
     * Implicit conversion for test ergonomics, allowing direct assignment of a value without specifying metadata.
     *
     * @param val The override value
     */
    // NOLINTNEXTLINE
    ConfigOverride(T val) : value{std::move(val)} {}

    /**
     * @brief Constructs an empty ConfigOverride from std::nullopt.
     *
     * @details
     * Implicit conversion for initializer list ergonomics, allowing @c std::nullopt in vector initializer lists.
     */
    // NOLINTNEXTLINE
    ConfigOverride(std::nullopt_t) {}

    /**
     * @brief Constructs a ConfigOverride with a value and full source metadata.
     *
     * @details
     * Used by config providers to set provider-specific source metadata during parsing.
     *
     * @param val The override value
     * @param source_key The provider-specific identifier (e.g., YAML path, CLI flag)
     * @param canonical_name Human-readable description of this override
     */
    ConfigOverride(T val, std::string source_key, std::string canonical_name)
        : value{std::move(val)}, source_key{std::move(source_key)}, canonical_name{std::move(canonical_name)}
    {
    }

    /**
     * @brief Constructs a ConfigOverride with a value, full source metadata, and per-entry source location.
     *
     * @details
     * Used by config providers to set provider-specific source metadata during parsing of map-type config values. The
     * @p source_info and @p source_details fields allow each entry within a map to carry its own source location, so
     * that @c ConfigValue::derive() can produce accurately located child values instead of inheriting the parent map's
     * location.
     *
     * @param val The override value
     * @param source_key The provider-specific identifier (e.g., YAML path, CLI flag)
     * @param canonical_name Human-readable description of this override
     * @param source_info Source location string (e.g., "./porytiles.yaml:42")
     * @param source_details Contextual highlight lines from the source file
     */
    ConfigOverride(
        T val,
        std::string source_key,
        std::string canonical_name,
        std::string source_info,
        std::vector<std::string> source_details)
        : value{std::move(val)}, source_key{std::move(source_key)}, canonical_name{std::move(canonical_name)},
          source_info{std::move(source_info)}, source_details{std::move(source_details)}
    {
    }

    /**
     * @brief Checks whether this override has a value set.
     *
     * @return @c true if a value is present
     */
    [[nodiscard]] bool has_value() const
    {
        return value.has_value();
    }

    /**
     * @brief Accesses the stored value.
     *
     * @pre has_value() must be @c true.
     * @return A const reference to the stored value
     */
    [[nodiscard]] const T &operator*() const
    {
        return *value;
    }
};

} // namespace porytiles2
