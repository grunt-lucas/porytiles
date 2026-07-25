#include "porytiles/domain/models/metatile_attribute_schema.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

namespace {

std::string hex_string(std::uint32_t mask)
{
    return std::format("0x{:X}", mask);
}

/// @brief Reports whether a mask is a single contiguous run of set bits.
///
/// @details
/// A contiguous run has as many set bits as it spans from its lowest to its highest set bit. The naive
/// "is (mask >> ctz) + 1 a power of two" test overflows for a full 32-bit mask, so the population count
/// against the span width is used instead. Assumes a non-zero mask, which the zero-mask rule guarantees
/// before this is called.
bool is_contiguous(std::uint32_t mask)
{
    return std::popcount(mask) == std::bit_width(mask) - std::countr_zero(mask);
}

} // namespace

EnumDefinition ProviderDefinition::to_enum_definition(std::string field_display_name, std::uint32_t max_value) const
{
    return EnumDefinition{
        .prefix = prefix,
        .max_value = max_value,
        .skipped = skipped,
        .format = format,
        .field_display_name = std::move(field_display_name)};
}

ChainableResult<Schema> Schema::create(std::vector<Field> fields, std::size_t attribute_bytes)
{
    assert_or_panic(
        attribute_bytes == 1 || attribute_bytes == 2 || attribute_bytes == 4,
        "Schema::create requires a 1-byte, 2-byte, or 4-byte attribute size");

    std::unordered_set<std::string> seen_names;
    std::optional<std::size_t> layer_type_index;

    for (std::size_t i = 0; i < fields.size(); ++i) {
        const Field &field = fields[i];
        const std::uint32_t mask = field.mask();

        if (seen_names.contains(field.name())) {
            return FormattableError{
                "Field '{}' is defined more than once in the schema.", FormatParam{field.name(), Style::bold}};
        }

        if (mask == 0) {
            return FormattableError{
                "Field '{}' has a zero mask: a field must occupy at least one bit.",
                FormatParam{field.name(), Style::bold}};
        }

        if (!is_contiguous(mask)) {
            return FormattableError{
                "Field '{}' has mask '{}': a field mask must be a single contiguous run of bits.",
                FormatParam{field.name(), Style::bold},
                FormatParam{hex_string(mask), Style::bold}};
        }

        if (static_cast<std::size_t>(std::bit_width(mask)) > attribute_bytes * 8) {
            return FormattableError{
                "Field '{}' has mask '{}', which extends beyond the '{}'-byte metatile attribute size.",
                FormatParam{field.name(), Style::bold},
                FormatParam{hex_string(mask), Style::bold},
                FormatParam{attribute_bytes, Style::bold}};
        }

        if (field.default_value() > field.max_value()) {
            return FormattableError{
                "Field '{}' has default value '{}', which does not fit in its {}-bit mask '{}'.",
                FormatParam{field.name(), Style::bold},
                FormatParam{field.default_value(), Style::bold},
                FormatParam{field.width()},
                FormatParam{hex_string(mask), Style::bold}};
        }

        // The layer_type role marks the one field whose values Porytiles manages (compile-time inference or a CSV
        // pin), so a provider or a default on it can never take effect and a second role field would be ambiguous.
        if (field.packs_layer_type()) {
            if (layer_type_index.has_value()) {
                return FormattableError{
                    "Field '{}' carries the layer_type role, but field '{}' already carries it. Only one field may "
                    "pack the layer type.",
                    FormatParam{field.name(), Style::bold},
                    FormatParam{fields[layer_type_index.value()].name(), Style::bold}};
            }
            if (field.has_provider()) {
                return FormattableError{
                    "Field '{}' carries the layer_type role, so it cannot have a value provider: its values are "
                    "managed by Porytiles.",
                    FormatParam{field.name(), Style::bold}};
            }
            if (field.default_value() != 0) {
                return FormattableError{
                    "Field '{}' carries the layer_type role, so it cannot have a default value: its values are "
                    "managed by Porytiles.",
                    FormatParam{field.name(), Style::bold}};
            }
            layer_type_index = i;
        }

        for (std::size_t j = 0; j < i; ++j) {
            const Field &prior = fields[j];
            if ((prior.mask() & mask) != 0) {
                return FormattableError{
                    "Field '{}' has mask '{}', which overlaps mask '{}' of field '{}'.",
                    FormatParam{field.name(), Style::bold},
                    FormatParam{hex_string(mask), Style::bold},
                    FormatParam{hex_string(prior.mask()), Style::bold},
                    FormatParam{prior.name(), Style::bold}};
            }
        }

        seen_names.insert(field.name());
    }

    return Schema{std::move(fields), attribute_bytes, layer_type_index};
}

} // namespace porytiles
