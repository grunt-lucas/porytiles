#pragma once

#include <format>
#include <ostream>
#include <string>
#include <vector>

#include "porytiles/domain/models/metatile_attribute_schema.hpp"

namespace porytiles {

/// @brief A user request to emit a trailing pin column for one schema role in attributes.csv.
///
/// @details
/// A role pin says "give this role its own trailing column in attributes.csv so its per-metatile values can be pinned
/// rather than inferred". Currently, the only role is layer_type, so a role pin activates a trailing layer-type column,
/// which is handled by the loader and writer. The column's header is not configurable: it is always
/// pin_column_name(role), i.e. "pin::layer_type". A fixed name is what lets the loader classify a header column from
/// its name alone, with no dependence on the schema or pin config that happens to be resolved at the time.
struct RolePinDefinition {
    FieldRole role;

    bool operator==(const RolePinDefinition &) const = default;
};

/// @brief An ordered list of role pin definitions; order is display/declaration order (also CSV column order).
using RolePinDefinitions = std::vector<RolePinDefinition>;

/// @brief Finds the role pin definition for a given role, or nullptr when the role is not pinned.
///
/// @param definitions The role pin list to search
/// @param role The role to look for
/// @return A pointer to the matching definition within @p definitions, or nullptr when none matches
[[nodiscard]] inline const RolePinDefinition *find_role_pin(const RolePinDefinitions &definitions, FieldRole role)
{
    for (const auto &definition : definitions) {
        if (definition.role == role) {
            return &definition;
        }
    }
    return nullptr;
}

/// @brief Converts one role pin definition to a human-readable string.
[[nodiscard]] inline std::string to_string(const RolePinDefinition &definition)
{
    return std::format("{{role={}}}", to_string(definition.role));
}

/// @brief Converts a role pin definition list to a human-readable string.
[[nodiscard]] inline std::string to_string(const RolePinDefinitions &definitions)
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

inline std::ostream &operator<<(std::ostream &os, const RolePinDefinitions &definitions)
{
    return os << to_string(definitions);
}

} // namespace porytiles

template <>
struct std::formatter<porytiles::RolePinDefinitions> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles::RolePinDefinitions &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles::to_string(value));
    }
};
