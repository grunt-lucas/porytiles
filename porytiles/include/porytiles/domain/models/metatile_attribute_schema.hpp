#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/**
 * @brief Describes which header declaration styles a provider header may use.
 *
 * @details
 * A base game exposes its metatile attribute value names (behavior constants, terrain types, and so on)
 * through a C header. Those names can be declared either as preprocessor defines, as C enum members, or
 * as a mix of both. A field's provider records which form is expected so the value scan can reject the
 * wrong one or accept either.
 */
enum class HeaderFormat { defines_only, enums_only, either };

/**
 * @brief Converts HeaderFormat to string representation.
 *
 * @param format The HeaderFormat to convert
 * @return String representation ("defines-only", "enums-only", or "either")
 */
[[nodiscard]] inline std::string to_string(HeaderFormat format)
{
    switch (format) {
    case HeaderFormat::defines_only:
        return "defines-only";
    case HeaderFormat::enums_only:
        return "enums-only";
    case HeaderFormat::either:
        return "either";
    }
    panic("unhandled HeaderFormat value");
}

/**
 * @brief Stream insertion operator for HeaderFormat.
 *
 * @param os The output stream
 * @param format The HeaderFormat to output
 * @return The output stream
 */
inline std::ostream &operator<<(std::ostream &os, const HeaderFormat format)
{
    return os << to_string(format);
}

struct EnumSpec;

/**
 * @brief Describes where and how a field's named values are declared.
 *
 * @details
 * A provider spec points a field at the header that declares its value names, the prefix those names
 * share, the names to skip during scanning, and which declaration styles are acceptable. It carries no
 * behavior of its own; it is a plain description consumed by later stages that actually read the header.
 */
struct ProviderSpec {
    std::filesystem::path header;
    std::string prefix;
    std::unordered_set<std::string> skipped{};
    HeaderFormat format{HeaderFormat::either};

    bool operator==(const ProviderSpec &) const = default;

    /**
     * @brief Builds the resolvable enum spec a header provider needs from this description.
     *
     * @details
     * A ProviderSpec describes a field's value names in the abstract; a provider that reads a header also
     * needs the field's value cap and a human-readable field name for diagnostics. Those two facts live on
     * the owning Field, not here, so the caller supplies them. The header path is intentionally not carried
     * over: it is resolved separately against the project root before a provider is built.
     *
     * @param field_display_name The field name used in this provider's diagnostics
     * @param max_value The largest value a resolved name may hold, from the field's width
     * @return An EnumSpec copying this spec's prefix, skip set, and format alongside the two given facts
     */
    [[nodiscard]] EnumSpec to_enum_spec(std::string field_display_name, std::uint32_t max_value) const;
};

/**
 * @brief The self-contained description a header enum provider needs to scan and validate values.
 *
 * @details
 * Where ProviderSpec describes how a field's values are declared in the abstract, an EnumSpec is the
 * concrete, resolvable form a provider consumes: it adds the field's value cap and a display name for
 * diagnostics, and drops the header path (resolved separately against the project root). It is plain data
 * derived from a ProviderSpec plus its owning Field via ProviderSpec::to_enum_spec.
 */
struct EnumSpec {
    std::string prefix;
    std::uint32_t max_value;
    std::unordered_set<std::string> skipped{};
    HeaderFormat format{HeaderFormat::either};
    std::string field_display_name;

    bool operator==(const EnumSpec &) const = default;
};

/**
 * @brief One named bit-field within a metatile attribute layout.
 *
 * @details
 * A field pairs a name with the mask of bits it occupies inside the packed attribute word. The mask is
 * the single source of truth for the field's position and size: the offset, width, and maximum storable
 * value are all derived from it. A field may also carry a provider spec describing how its values are
 * named in a base-game header.
 *
 * A Field is a passive carrier and does not validate itself. Layout rules (contiguous non-zero mask,
 * in-range, non-overlapping, default fits) are enforced when a Field is placed into a Schema via
 * Schema::create. Deriving offset() or max_value() from a zero mask is meaningless, so those accessors
 * document a non-zero-mask precondition that Schema::create guarantees for every field it accepts.
 *
 * @invariant For a Field obtained from a created Schema, mask() is a single contiguous run of at least one bit.
 */
class Field {
  public:
    /**
     * @brief Constructs a field from a name, mask, and optional default value and provider spec.
     *
     * @param name The field name
     * @param mask The bits this field occupies within the packed attribute word
     * @param default_value The value used when the field is absent from a metatile's attribute
     * @param provider An optional description of how this field's named values are declared
     */
    Field(
        std::string name,
        std::uint32_t mask,
        std::uint32_t default_value = 0,
        std::optional<ProviderSpec> provider = std::nullopt)
        : name_{std::move(name)}, mask_{mask}, default_value_{default_value}, provider_{std::move(provider)}
    {
    }

    [[nodiscard]] const std::string &name() const
    {
        return name_;
    }

    [[nodiscard]] std::uint32_t mask() const
    {
        return mask_;
    }

    [[nodiscard]] std::uint32_t default_value() const
    {
        return default_value_;
    }

    /**
     * @brief Returns the bit offset of the field's least-significant bit.
     *
     * @pre mask() must not be zero. Guaranteed for any field inside a created Schema.
     * @return The number of low-order zero bits in the mask
     */
    [[nodiscard]] std::uint32_t offset() const
    {
        return static_cast<std::uint32_t>(std::countr_zero(mask_));
    }

    /**
     * @brief Returns the number of bits the field occupies.
     *
     * @return The population count of the mask
     */
    [[nodiscard]] std::uint32_t width() const
    {
        return static_cast<std::uint32_t>(std::popcount(mask_));
    }

    /**
     * @brief Returns the largest value the field can hold.
     *
     * @details
     * This is the mask shifted down to bit zero, which is the single source of truth for the range checks
     * that Schema::create and validate_provider_values apply.
     *
     * @pre mask() must not be zero. Guaranteed for any field inside a created Schema.
     * @return The maximum value representable in the field's width
     */
    [[nodiscard]] std::uint32_t max_value() const
    {
        return mask_ >> offset();
    }

    [[nodiscard]] bool has_provider() const
    {
        return provider_.has_value();
    }

    /**
     * @brief Returns the field's provider spec.
     *
     * @pre has_provider() must be true.
     * @return A const reference to the provider spec
     */
    [[nodiscard]] const ProviderSpec &provider_spec() const
    {
        assert_or_panic(provider_.has_value(), "Field::provider_spec() called on a field with no provider");
        return provider_.value();
    }

    /**
     * @brief Checks that a set of named values all fit within this field's width.
     *
     * @details
     * The entries are given in header order as (name, value) pairs so the first offender is reported
     * deterministically. The check depends only on the field's width, not on whether it has a provider,
     * which keeps it usable with synthetic fields in tests. The header path is deliberately not part of
     * the message; callers chain file context around this via the PT_TRY_* macros.
     *
     * @param entries The named values to check, in header declaration order
     * @return An empty result on success, or an error naming the first value that does not fit
     */
    [[nodiscard]] ChainableResult<void>
    validate_provider_values(const std::vector<std::pair<std::string, std::uint32_t>> &entries) const;

  private:
    std::string name_;
    std::uint32_t mask_;
    std::uint32_t default_value_;
    std::optional<ProviderSpec> provider_;
};

/**
 * @brief A validated metatile attribute layout: an ordered set of non-overlapping fields.
 *
 * @details
 * A Schema is the data-driven replacement for hardcoded per-base-game attribute layouts. It owns the
 * fields that make up an attribute word and the byte size those fields were validated against. Schemas
 * can only be built through Schema::create, which enforces the layout rules, so any Schema in hand is
 * known to be well-formed: every field has a contiguous non-zero mask that fits the attribute size, no
 * two fields overlap, no name repeats, and every default fits its field.
 */
class Schema {
  public:
    /**
     * @brief Validates a set of fields against an attribute size and builds a Schema.
     *
     * @details
     * Runs a single fail-fast pass over the fields in the given order. Intra-field rules (zero mask,
     * non-contiguous mask, mask beyond the attribute size, default value too large) are checked before
     * the cross-field overlap and duplicate-name rules. The first violation wins and is returned as the
     * error; on success the fields are stored in the order given.
     *
     * @param fields The fields making up the layout, in the order they should be preserved
     * @param attr_bytes The attribute size in bytes the layout is validated against
     * @pre @p attr_bytes must be 2 or 4.
     * @return A validated Schema, or an error describing the first layout rule violation
     */
    [[nodiscard]] static ChainableResult<Schema> create(std::vector<Field> fields, std::size_t attr_bytes);

    [[nodiscard]] const std::vector<Field> &fields() const
    {
        return fields_;
    }

    [[nodiscard]] std::size_t attr_bytes() const
    {
        return attr_bytes_;
    }

  private:
    Schema(std::vector<Field> fields, std::size_t attr_bytes) : fields_{std::move(fields)}, attr_bytes_{attr_bytes} {}

    std::vector<Field> fields_;
    std::size_t attr_bytes_;
};

} // namespace porytiles

template <>
struct std::formatter<porytiles::HeaderFormat> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles::HeaderFormat &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles::to_string(value));
    }
};
