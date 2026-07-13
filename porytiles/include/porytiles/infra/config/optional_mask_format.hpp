#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>

/// @brief Formats an optional 32-bit mask config value for display.
///
/// @details
/// C++23's std::format has no built-in formatter for std::optional, but the config system formats every
/// config value's type via std::format (in dump-config output, provenance chains, and diagnostics). The
/// layer-type mask config values are std::optional<std::uint32_t>, where an engaged value is a hex mask
/// and std::nullopt means "unset" (resolve from the base game, then the size-based default). This
/// specialization renders present values as 0xHEX and an unset value as the literal "unset".
template <>
struct std::formatter<std::optional<std::uint32_t>> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const std::optional<std::uint32_t> &value, auto &ctx) const
    {
        if (value.has_value()) {
            return std::format_to(ctx.out(), "0x{:X}", value.value());
        }
        return std::format_to(ctx.out(), "unset");
    }
};

/// @brief Formats an optional size config value for display.
///
/// @details
/// Companion to the optional mask formatter above, for config values typed std::optional<std::size_t> (currently the
/// metatile attribute declaration size). An engaged value is a plain byte count, so it renders as decimal; an unset
/// value renders as the literal "unset" (meaning "fall back to the related size value").
template <>
struct std::formatter<std::optional<std::size_t>> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const std::optional<std::size_t> &value, auto &ctx) const
    {
        if (value.has_value()) {
            return std::format_to(ctx.out(), "{}", value.value());
        }
        return std::format_to(ctx.out(), "unset");
    }
};
