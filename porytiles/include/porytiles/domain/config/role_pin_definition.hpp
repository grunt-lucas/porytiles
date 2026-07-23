#pragma once

#include <format>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

class TextFormatter;

/// @brief A user request to emit a trailing pin column for one schema role in attributes.csv.
///
/// @details
/// A role pin says "give this role its own trailing column in attributes.csv so its per-metatile values can be pinned
/// rather than inferred". Currently, the only role is layer_type, so a role pin activates a trailing layer-type column,
/// which is handled by the loader and writer. `role` names the target role. `column` is the optional column header;
/// when absent the header defaults to the role's string form (`to_string(role)`, i.e. "layer_type").
struct RolePinDefinition {
    FieldRole role;
    std::optional<std::string> column;

    bool operator==(const RolePinDefinition &) const = default;
};

/// @brief An ordered list of role pin definitions; order is display/declaration order (also CSV column order).
using RolePinDefinitions = std::vector<RolePinDefinition>;

/// @brief Returns the effective column header for a role pin: its explicit `column`, or the role's string form.
///
/// @param definition The role pin definition to resolve
/// @return The configured column name when present, otherwise to_string(definition.role)
[[nodiscard]] inline std::string effective_pin_column_name(const RolePinDefinition &definition)
{
    return definition.column.value_or(to_string(definition.role));
}

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

/// @brief Validates role pins against the active schema and errors on a column that collides with a schema name.
///
/// @details
/// The schema-independent role-pin checks (unknown/duplicate role, malformed column, collision with "id" or another
/// entry) run in the YAML parser, where the diagnostic can point at the offending file and line. This check needs the
/// resolved schema, so it runs later in both the loader and the writer: an effective pin column name that duplicates
/// any value field name would make the CSV ambiguous, so it is rejected. A role pin for a role the schema does not
/// carry is allowed (it still controls dual-layerize within Porytiles, even with no packed layer_type field in the
/// output).
///
/// @todo If role pins ever come from a source other than the YAML parser (e.g. OverrideConfigProvider::set_role_pins),
/// the schema-independent invariants above go unchecked. Add a schema-independent validator for that path.
///
/// @param definitions The resolved role pins to validate
/// @param schema The active attribute schema whose value field names must not collide with a pin column
/// @param format The formatter used to render the error message
/// @return An empty success result, or a FormattableError describing the first collision
[[nodiscard]] ChainableResult<void> validate_role_pins_against_schema(
    const RolePinDefinitions &definitions, const Schema &schema, const TextFormatter &format);

/// @brief Converts one role pin definition to a human-readable string.
[[nodiscard]] inline std::string to_string(const RolePinDefinition &definition)
{
    std::string result = std::format("{{role={}", to_string(definition.role));
    if (definition.column.has_value()) {
        result += ", column=" + definition.column.value();
    }
    result += "}";
    return result;
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
