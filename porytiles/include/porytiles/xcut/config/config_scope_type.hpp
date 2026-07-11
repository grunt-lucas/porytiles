#pragma once

#include <optional>
#include <string>

namespace porytiles {

/// @brief Specifies the scope type for configuration value lookups.
///
/// @details
/// ConfigScopeType determines how configuration providers should interpret the scope parameter when fetching
/// configuration values. This allows the same configuration key to have different values depending on whether it's
/// being used in a tileset or layout context.
///
/// For example, the YAML provider might look for tileset-specific configuration in
/// `data/tilesets/primary/<tileset_dir>/porytiles.yaml` when using ConfigScopeType::tileset, while layout-specific
/// configuration might be loaded from the layout directory when using ConfigScopeType::layout.
enum class ConfigScopeType {
    /// @brief Configuration scoped to a specific tileset.
    ///
    /// @details
    /// When using this scope type, the scope parameter should be the tileset name, and providers will look for
    /// tileset-specific configuration values.
    tileset,

    /// @brief Configuration scoped to a specific layout.
    ///
    /// @details
    /// When using this scope type, the scope parameter should be the layout name, and providers will look for
    /// layout-specific configuration values.
    ///
    /// @note Layout configuration support is not yet fully implemented in all providers.
    layout
};

/// @brief Converts a ConfigScopeType enum value to its string representation.
///
/// @details
/// This function provides a canonical string representation of the ConfigScopeType enum. Useful for logging, debugging,
/// cache key generation, and serialization.
///
/// @param type The ConfigScopeType value to convert
/// @return The string representation ("tileset" or "layout")
///
/// @note This function always returns a valid string for any ConfigScopeType value.
[[nodiscard]] inline std::string to_string(ConfigScopeType type)
{
    switch (type) {
    case ConfigScopeType::tileset:
        return "tileset";
    case ConfigScopeType::layout:
        return "layout";
    }
    // Should never reach here with valid enum value
    return "unknown";
}

/// @brief Parses a string into a ConfigScopeType enum value.
///
/// @details
/// This function attempts to parse a string representation into its corresponding ConfigScopeType enum value. The
/// parsing is case-sensitive and expects exact matches for "tileset" or "layout".
///
/// @param str The string to parse
/// @return An optional containing the parsed ConfigScopeType, or std::nullopt if the string doesn't match any known
/// scope type
///
/// @note This function performs exact string matching. "Tileset" or "TILESET" will not match.
[[nodiscard]] inline std::optional<ConfigScopeType> from_string(const std::string &str)
{
    if (str == "tileset") {
        return ConfigScopeType::tileset;
    }
    if (str == "layout") {
        return ConfigScopeType::layout;
    }
    return std::nullopt;
}

} // namespace porytiles
