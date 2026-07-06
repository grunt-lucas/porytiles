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

} // namespace

ChainableResult<void>
Field::validate_provider_values(const std::vector<std::pair<std::string, std::uint32_t>> &entries) const
{
    const std::uint32_t max = max_value();
    for (const auto &[value_name, value] : entries) {
        if (value > max) {
            return FormattableError{
                "'{}' has value '{}', which does not fit in the {}-bit field '{}'.",
                FormatParam{value_name, Style::bold},
                FormatParam{value, Style::bold},
                FormatParam{width()},
                FormatParam{name_, Style::bold}};
        }
    }
    return {};
}

ChainableResult<Schema> Schema::create(std::vector<Field> fields, std::size_t attr_bytes)
{
    assert_or_panic(attr_bytes == 2 || attr_bytes == 4, "Schema::create requires a 2-byte or 4-byte attribute size");

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

    return Schema{std::move(fields), attr_bytes};
}

} // namespace porytiles
