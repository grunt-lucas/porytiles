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
/// This is the raw config-layer value that feeds the schema loader. The loader turns the definitions into a validated
/// Schema and resolves any overrides.
///
/// @c mask is the field's bit mask; it is optional at the parse layer so an override merge can supply it, but a fully
/// merged definition without one is rejected by the loader. @c default_value is the value used when the field is absent
/// from a metatile. @c provider optionally points the field at the header that declares its value names. @c role
/// marks a field whose per-metatile values Porytiles manages (see FieldRole); in YAML it is spelled
/// `role: layer_type`.
struct MetatileAttributeFieldDefinition {
    std::string name;
    std::optional<std::uint32_t> mask;
    std::optional<std::uint32_t> default_value;
    std::optional<ProviderDefinition> provider;
    std::optional<FieldRole> role;

    bool operator==(const MetatileAttributeFieldDefinition &) const = default;
};

/// @brief An ordered list of metatile attribute field definitions; order is display/declaration order.
using MetatileAttributeFieldDefinitions = std::vector<MetatileAttributeFieldDefinition>;

/// @brief A partial override of a field's provider definition.
///
/// @details
/// Each present member replaces the corresponding member of the base provider definition at merge time; absent members
/// leave the base value untouched. @c skipped, when present, replaces the base skip set wholesale rather than adding to
/// it.
/// @c remove encodes `provider: null`, which drops the field's provider entirely (turning it into a raw field).
struct ProviderDefinitionOverride {
    bool remove{false};
    std::optional<std::filesystem::path> header;
    std::optional<std::string> prefix;
    std::optional<std::unordered_set<std::string>> skipped;
    std::optional<HeaderFormat> format;

    bool operator==(const ProviderDefinitionOverride &) const = default;
};

/// @brief A partial override of a single field, merged additively onto a baseline definition.
///
/// @details
/// Present scalar members replace the baseline field's value; absent members fall through. @c provider is itself a
/// partial override: nullopt means "do not touch the provider", while a present value adjusts (or removes) it. @c
/// role is a two-level optional: an absent outer value leaves the baseline role untouched, while a present outer
/// value replaces it, where an inner nullopt encodes `role: null` (clearing the role) and an inner value encodes
/// `role: layer_type` (setting it).
struct MetatileAttributeFieldOverride {
    std::optional<std::uint32_t> mask;
    std::optional<std::uint32_t> default_value;
    std::optional<ProviderDefinitionOverride> provider;
    std::optional<std::optional<FieldRole>> role;

    bool operator==(const MetatileAttributeFieldOverride &) const = default;
};

/// @brief A map from field name to its override; applied at schema load time.
using MetatileAttributeFieldOverrides = std::map<std::string, MetatileAttributeFieldOverride>;

namespace detail {

[[nodiscard]] inline std::string format_optional_mask(const std::optional<std::uint32_t> &mask)
{
    return mask.has_value() ? std::format("0x{:X}", mask.value()) : "none";
}

[[nodiscard]] inline std::string format_provider_definition(const ProviderDefinition &provider)
{
    return std::format(
        "{{header={}, prefix={}, skip_count={}, format={}}}",
        provider.header.string(),
        provider.prefix,
        provider.skipped.size(),
        to_string(provider.format));
}

} // namespace detail

/// @brief Converts one field definition to a human-readable string.
[[nodiscard]] inline std::string to_string(const MetatileAttributeFieldDefinition &definition)
{
    std::string result = std::format(
        "{}={{mask={}, default={}",
        definition.name,
        detail::format_optional_mask(definition.mask),
        detail::format_optional_mask(definition.default_value));
    if (definition.provider.has_value()) {
        result += ", provider=" + detail::format_provider_definition(definition.provider.value());
    }
    if (definition.role.has_value()) {
        result += ", role=" + to_string(definition.role.value());
    }
    result += "}";
    return result;
}

/// @brief Converts a field definition list to a human-readable string.
[[nodiscard]] inline std::string to_string(const MetatileAttributeFieldDefinitions &definitions)
{
    if (definitions.empty()) {
        return "[]";
    }
    std::string result = "[";
    bool first = true;
    for (const auto &definition : definitions) {
        if (!first) {
            result += ", ";
        }
        result += to_string(definition);
        first = false;
    }
    result += "]";
    return result;
}

inline std::ostream &operator<<(std::ostream &os, const MetatileAttributeFieldDefinitions &definitions)
{
    return os << to_string(definitions);
}

/// @brief Converts one field override to a human-readable string.
[[nodiscard]] inline std::string to_string(const MetatileAttributeFieldOverride &override_value)
{
    std::string result = std::format(
        "{{mask={}, default={}",
        detail::format_optional_mask(override_value.mask),
        detail::format_optional_mask(override_value.default_value));
    if (override_value.provider.has_value()) {
        if (override_value.provider->remove) {
            result += ", provider=removed";
        }
        else {
            result += ", provider=adjusted";
        }
    }
    if (override_value.role.has_value()) {
        if (override_value.role->has_value()) {
            result += ", role=" + to_string(override_value.role->value());
        }
        else {
            result += ", role=removed";
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
struct std::formatter<porytiles::MetatileAttributeFieldDefinitions> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles::MetatileAttributeFieldDefinitions &value, auto &ctx) const
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
