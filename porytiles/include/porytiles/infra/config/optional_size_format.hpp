#pragma once

#include <cstddef>
#include <format>
#include <optional>

/// @brief Formats an optional size config value for display.
///
/// @details
/// C++23's std::format has no built-in formatter for std::optional, but the config system formats every config value's
/// type via std::format (in dump-config output, provenance chains, and diagnostics). The metatile attribute size and
/// declaration size are std::optional<std::size_t>, where an engaged value is a byte count and std::nullopt means
/// "unset" (derive the value instead). This specialization renders a present value as decimal and an unset value as
/// the literal "unset".
///
/// std::optional<std::size_t> has no program-defined component, so [namespace.std] does not sanction specializing
/// std::formatter for it. It is done here because a std::formatter specialization is the config system's only
/// extension point for rendering a value type (see FormatParam::resolve_string), and the alternative is threading a
/// program-defined wrapper type through the whole generated provider chain. Should a future standard library format
/// std::optional itself, this becomes a redefinition error rather than a silent behavior change. Keep it the only
/// std::optional specialization in the tree: two of them collide outright on any platform where their wrapped types
/// are the same type (std::size_t and std::uint32_t are both unsigned int on 32-bit targets, for instance).
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
