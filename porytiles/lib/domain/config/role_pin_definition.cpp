#include "porytiles/domain/config/role_pin_definition.hpp"

#include <string>

#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/utilities/result/error.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

ChainableResult<void> validate_role_pins_against_schema(
    const RolePinDefinitions &definitions, const Schema &schema, const TextFormatter &format)
{
    // TODO: role pins currently only come from the YAML parser, which enforces the schema-independent invariants
    // (unknown/duplicate role, duplicate or reserved "id" column, malformed column names). If another source ever
    // produces pins (e.g. OverrideConfigProvider::set_role_pins), those invariants would go unchecked; add a
    // schema-independent validate_role_pins for that path.
    for (const auto &definition : definitions) {
        const std::string column = effective_pin_column_name(definition);
        for (const Field &field : schema.value_fields()) {
            if (field.name() == column) {
                return FormattableError{format.format(
                    "Role pin column '{}' collides with the '{}' attribute value field; give the {} role pin a "
                    "different 'column' value.",
                    FormatParam{column, Style::bold},
                    FormatParam{field.name(), Style::bold},
                    FormatParam{to_string(definition.role), Style::bold})};
            }
        }
    }
    return {};
}

} // namespace porytiles
