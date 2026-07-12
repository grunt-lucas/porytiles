#pragma once

#include <cstdint>
#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_set>
#include <vector>

#include "porytiles/domain/models/metatile_attribute_schema.hpp"

namespace porytiles {

/// @brief A user-authored (or inferred) description of one metatile attribute field.
///
/// @details
/// This is the raw config-layer value that feeds the schema loader. Unlike the validated domain Field, a spec may be
/// incomplete: a field with only a @c frlg_mask and no primary @c mask is an alternate-layout-only field, valid at the
/// config layer but excluded from the primary Schema. The loader turns the specs into a validated Schema and resolves
/// any overrides.
///
/// @c mask is the primary-layout bit mask; when absent, the field is alternate-only. @c frlg_mask is the FRLG-layout
/// mask stored for per-tileset layout selection (see issue #283). @c default_value is the value used when the field is
/// absent from a metatile. @c provider optionally points the field at the header that declares its value names.
struct MetatileAttributeFieldSpec {
    std::string name;
    std::optional<std::uint32_t> mask;
    std::optional<std::uint32_t> frlg_mask;
    std::optional<std::uint32_t> default_value;
    std::optional<ProviderSpec> provider;

    bool operator==(const MetatileAttributeFieldSpec &) const = default;
};

/// @brief An ordered list of metatile attribute field specs; order is display/declaration order.
using MetatileAttributeFieldSpecs = std::vector<MetatileAttributeFieldSpec>;

/// @brief A partial override of a field's provider spec.
///
/// @details
/// Each present member replaces the corresponding member of the base provider spec at merge time; absent members leave
/// the base value untouched. @c skipped, when present, replaces the base skip set wholesale rather than adding to it.
/// @c remove encodes `provider: null`, which drops the field's provider entirely (turning it into a raw field).
struct ProviderSpecOverride {
    bool remove{false};
    std::optional<std::filesystem::path> header;
    std::optional<std::string> prefix;
    std::optional<std::unordered_set<std::string>> skipped;
    std::optional<HeaderFormat> format;

    bool operator==(const ProviderSpecOverride &) const = default;
};

/// @brief A partial override of a single field, merged additively onto a baseline spec.
///
/// @details
/// Present scalar members replace the baseline field's value; absent members fall through. @c provider is itself a
/// partial override: nullopt means "do not touch the provider", while a present value adjusts (or removes) it.
struct MetatileAttributeFieldOverride {
    std::optional<std::uint32_t> mask;
    std::optional<std::uint32_t> frlg_mask;
    std::optional<std::uint32_t> default_value;
    std::optional<ProviderSpecOverride> provider;

    bool operator==(const MetatileAttributeFieldOverride &) const = default;
};

/// @brief A map from field name to its override; applied at schema load time.
using MetatileAttributeFieldOverrides = std::map<std::string, MetatileAttributeFieldOverride>;

namespace detail {

[[nodiscard]] inline std::string format_optional_mask(const std::optional<std::uint32_t> &mask)
{
    return mask.has_value() ? std::format("0x{:X}", mask.value()) : "none";
}

[[nodiscard]] inline std::string format_provider_spec(const ProviderSpec &provider)
{
    return std::format(
        "{{header={}, prefix={}, skip_count={}, format={}}}",
        provider.header.string(),
        provider.prefix,
        provider.skipped.size(),
        to_string(provider.format));
}

} // namespace detail

/// @brief Converts one field spec to a human-readable string.
[[nodiscard]] inline std::string to_string(const MetatileAttributeFieldSpec &spec)
{
    std::string result = std::format(
        "{}={{mask={}, frlg_mask={}, default={}",
        spec.name,
        detail::format_optional_mask(spec.mask),
        detail::format_optional_mask(spec.frlg_mask),
        detail::format_optional_mask(spec.default_value));
    if (spec.provider.has_value()) {
        result += ", provider=" + detail::format_provider_spec(spec.provider.value());
    }
    result += "}";
    return result;
}

/// @brief Converts a field spec list to a human-readable string.
[[nodiscard]] inline std::string to_string(const MetatileAttributeFieldSpecs &specs)
{
    if (specs.empty()) {
        return "[]";
    }
    std::string result = "[";
    bool first = true;
    for (const auto &spec : specs) {
        if (!first) {
            result += ", ";
        }
        result += to_string(spec);
        first = false;
    }
    result += "]";
    return result;
}

inline std::ostream &operator<<(std::ostream &os, const MetatileAttributeFieldSpecs &specs)
{
    return os << to_string(specs);
}

/// @brief Converts one field override to a human-readable string.
[[nodiscard]] inline std::string to_string(const MetatileAttributeFieldOverride &override_value)
{
    std::string result = std::format(
        "{{mask={}, frlg_mask={}, default={}",
        detail::format_optional_mask(override_value.mask),
        detail::format_optional_mask(override_value.frlg_mask),
        detail::format_optional_mask(override_value.default_value));
    if (override_value.provider.has_value()) {
        if (override_value.provider->remove) {
            result += ", provider=removed";
        }
        else {
            result += ", provider=adjusted";
        }
    }
    result += "}";
    return result;
}

/// @brief Converts a field override map to a human-readable string.
[[nodiscard]] inline std::string to_string(const MetatileAttributeFieldOverrides &overrides)
{
    if (overrides.empty()) {
        return "{}";
    }
    std::string result = "{";
    bool first = true;
    for (const auto &[name, override_value] : overrides) {
        if (!first) {
            result += ", ";
        }
        result += name + "=" + to_string(override_value);
        first = false;
    }
    result += "}";
    return result;
}

inline std::ostream &operator<<(std::ostream &os, const MetatileAttributeFieldOverrides &overrides)
{
    return os << to_string(overrides);
}

} // namespace porytiles

template <>
struct std::formatter<porytiles::MetatileAttributeFieldSpecs> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles::MetatileAttributeFieldSpecs &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles::to_string(value));
    }
};

template <>
struct std::formatter<porytiles::MetatileAttributeFieldOverrides> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles::MetatileAttributeFieldOverrides &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles::to_string(value));
    }
};
