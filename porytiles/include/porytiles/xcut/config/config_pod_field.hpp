#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace porytiles {

/// @brief A lightweight wrapper for per-field configuration values with source metadata.
///
/// @details
/// ConfigPODField replaces @c std::optional<T> in configuration POD structs where individual fields need to carry
/// provider-specific source metadata (source_key, canonical_name). This metadata is set at parse time by the provider
/// (e.g., YAML parser, CLI parser) and later used by @c ConfigValue::derive() to construct a child @c ConfigValue<T>
/// with proper provenance.
///
/// Implicit constructors from @c T and @c std::nullopt_t preserve ergonomic usage in tests and initializer lists.
///
/// @tparam T The type of the field value
template <typename T>
struct ConfigPODField {
    std::optional<T> value{std::nullopt};
    std::string source_key;
    std::string canonical_name;
    std::string source_info;
    std::vector<std::string> source_details;

    ConfigPODField() = default;

    /// @brief Constructs a ConfigPODField with a value and no source metadata.
    ///
    /// @details
    /// Implicit conversion for test ergonomics, allowing direct assignment of a value without specifying metadata.
    ///
    /// @param val The field value
    // NOLINTNEXTLINE
    ConfigPODField(T val) : value{std::move(val)} {}

    /// @brief Constructs an empty ConfigPODField from std::nullopt.
    ///
    /// @details
    /// Implicit conversion for initializer list ergonomics, allowing @c std::nullopt in vector initializer lists.
    // NOLINTNEXTLINE
    ConfigPODField(std::nullopt_t) {}

    /// @brief Constructs a ConfigPODField with a value and full source metadata.
    ///
    /// @details
    /// Used by config providers to set provider-specific source metadata during parsing.
    ///
    /// @param val The field value
    /// @param source_key The provider-specific identifier (e.g., YAML path, CLI flag)
    /// @param canonical_name Human-readable description of this field
    ConfigPODField(T val, std::string source_key, std::string canonical_name)
        : value{std::move(val)}, source_key{std::move(source_key)}, canonical_name{std::move(canonical_name)}
    {
    }

    /// @brief Constructs a ConfigPODField with a value, full source metadata, and per-entry source location.
    ///
    /// @details
    /// Used by config providers to set provider-specific source metadata during parsing of map-type config values. The
    /// @p source_info and @p source_details fields allow each entry within a map to carry its own source location, so
    /// that @c ConfigValue::derive() can produce accurately located child values instead of inheriting the parent map's
    /// location.
    ///
    /// @param val The field value
    /// @param source_key The provider-specific identifier (e.g., YAML path, CLI flag)
    /// @param canonical_name Human-readable description of this field
    /// @param source_info Source location string (e.g., "./porytiles.yaml:42")
    /// @param source_details Contextual highlight lines from the source file
    ConfigPODField(
        T val,
        std::string source_key,
        std::string canonical_name,
        std::string source_info,
        std::vector<std::string> source_details)
        : value{std::move(val)}, source_key{std::move(source_key)}, canonical_name{std::move(canonical_name)},
          source_info{std::move(source_info)}, source_details{std::move(source_details)}
    {
    }

    /// @brief Checks whether this field has a value set.
    ///
    /// @return @c true if a value is present
    [[nodiscard]] bool has_value() const
    {
        return value.has_value();
    }

    /// @brief Accesses the stored value.
    ///
    /// @pre has_value() must be @c true.
    /// @return A const reference to the stored value
    [[nodiscard]] const T &operator*() const
    {
        return *value;
    }
};

} // namespace porytiles
