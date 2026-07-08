#include "porytiles/domain/models/metatile_attribute_schema.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <format>
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

/**
 * @brief Reports whether a mask is a single contiguous run of set bits.
 *
 * @details
 * A contiguous run has as many set bits as it spans from its lowest to its highest set bit. The naive
 * "is (mask >> ctz) + 1 a power of two" test overflows for a full 32-bit mask, so the population count
 * against the span width is used instead. Assumes a non-zero mask, which the zero-mask rule guarantees
 * before this is called.
 */
bool is_contiguous(std::uint32_t mask)
{
    return std::popcount(mask) == std::bit_width(mask) - std::countr_zero(mask);
}

/**
 * @brief Returns the size-convention layer_type mask for the given attribute width.
 *
 * @details
 * When no explicit layer_type mask is configured or inferred, Porytiles falls back to the fixed
 * GBA/porymap format convention keyed on the total attribute width: bits 12-15 in a 2-byte attribute
 * word, bits 29-30 in a 4-byte word, and disabled (0) in a 1-byte word (there is no vanilla 1-byte
 * layer-type position). This helper is the single named home for that convention; Schema::create uses
 * it only as the fallback when the caller passes std::nullopt.
 */
std::uint32_t structural_layer_type_mask(std::size_t attr_bytes)
{
    switch (attr_bytes) {
    case 4:
        return 0x60000000U;
    case 2:
        return 0x0000F000U;
    default: // 1-byte: no vanilla layer-type convention, so disabled
        return 0U;
    }
}

} // namespace

EnumSpec ProviderSpec::to_enum_spec(std::string field_display_name, std::uint32_t max_value) const
{
    return EnumSpec{
        .prefix = prefix,
        .max_value = max_value,
        .skipped = skipped,
        .format = format,
        .field_display_name = std::move(field_display_name)};
}

ChainableResult<Schema>
Schema::create(std::vector<Field> fields, std::size_t attr_bytes, std::optional<std::uint32_t> layer_type_mask)
{
    assert_or_panic(
        attr_bytes == 1 || attr_bytes == 2 || attr_bytes == 4,
        "Schema::create requires a 1-byte, 2-byte, or 4-byte attribute size");

    // An explicit mask (including 0, which disables the layer type) wins; otherwise fall back to the size convention.
    const std::uint32_t ltm = layer_type_mask.value_or(structural_layer_type_mask(attr_bytes));

    // A non-zero layer_type mask is itself part of the layout, so it must obey the same shape rules as a field. A
    // zero mask means the layer type is disabled, so these checks are skipped and it never overlaps anything.
    if (ltm != 0) {
        if (!is_contiguous(ltm)) {
            return FormattableError{
                "The layer type mask '{}' must be a single contiguous run of bits.",
                FormatParam{hex_string(ltm), Style::bold}};
        }
        if (static_cast<std::size_t>(std::bit_width(ltm)) > attr_bytes * 8) {
            return FormattableError{
                "The layer type mask '{}' extends beyond the '{}'-byte metatile attribute size.",
                FormatParam{hex_string(ltm), Style::bold},
                FormatParam{attr_bytes, Style::bold}};
        }
    }

    std::unordered_set<std::string> seen_names;

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

        if (static_cast<std::size_t>(std::bit_width(mask)) > attr_bytes * 8) {
            return FormattableError{
                "Field '{}' has mask '{}', which extends beyond the '{}'-byte metatile attribute size.",
                FormatParam{field.name(), Style::bold},
                FormatParam{hex_string(mask), Style::bold},
                FormatParam{attr_bytes, Style::bold}};
        }

        if (ltm != 0 && (mask & ltm) != 0) {
            return FormattableError{
                "Field '{}' has mask '{}', which overlaps the layer type bits '{}' of a {}-byte metatile attribute.",
                FormatParam{field.name(), Style::bold},
                FormatParam{hex_string(mask), Style::bold},
                FormatParam{hex_string(ltm), Style::bold},
                FormatParam{attr_bytes, Style::bold}};
        }

        if (field.default_value() > field.max_value()) {
            return FormattableError{
                "Field '{}' has default value '{}', which does not fit in its {}-bit mask '{}'.",
                FormatParam{field.name(), Style::bold},
                FormatParam{field.default_value(), Style::bold},
                FormatParam{field.width()},
                FormatParam{hex_string(mask), Style::bold}};
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

    return Schema{std::move(fields), attr_bytes, ltm};
}

} // namespace porytiles
