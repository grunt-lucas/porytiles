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

/// @brief Describes which header declaration styles a provider header may use.
///
/// @details
/// Decomp projects typically expose metatile attribute value names (behavior constants, terrain types, and so on)
/// through a C header. Those names can be declared either as preprocessor defines, as C enum members, or as a mix of
/// both. A field's provider records which form is expected so the value scan can reject the wrong one or accept either.
enum class HeaderFormat { defines_only, enums_only, either };

/// @brief Converts HeaderFormat to string representation.
///
/// @param format The HeaderFormat to convert
/// @return The string representation
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

/// @brief Stream insertion operator for HeaderFormat.
///
/// @param os The output stream
/// @param format The HeaderFormat to output
/// @return The output stream
inline std::ostream &operator<<(std::ostream &os, const HeaderFormat format)
{
    return os << to_string(format);
}

/// @brief Semantic roles a schema field can carry beyond holding a plain per-metatile value.
///
/// @details
/// Most fields are plain values: the attributes CSV supplies them and the compiler packs them as written.  A role marks
/// a field whose values Porytiles can manage itself. The only role today is layer_type: the field that receives each
/// metatile's layer type (inferred at compile time, or pinned via the CSV's trailing layer_type column). A role-bearing
/// field never has a value provider or a default, and it is excluded from the normal attributes CSV columns.
enum class FieldRole { layer_type };

/// @brief Converts FieldRole to string representation.
///
/// @param role The FieldRole to convert
/// @return String representation ("layer_type")
[[nodiscard]] inline std::string to_string(FieldRole role)
{
    switch (role) {
    case FieldRole::layer_type:
        return "layer_type";
    }
    panic("unhandled FieldRole value");
}

/// @brief Stream insertion operator for FieldRole.
///
/// @param os The output stream
/// @param role The FieldRole to output
/// @return The output stream
inline std::ostream &operator<<(std::ostream &os, const FieldRole role)
{
    return os << to_string(role);
}

struct EnumDefinition;

/// @brief Describes where and how a field's named values are declared.
///
/// @details
/// A provider definition points a field at the header that declares its value names, the prefix those names
/// share, the names to skip during scanning, and which declaration styles are acceptable. It carries no
/// behavior of its own; it is a plain description consumed by later stages that actually read the header.
struct ProviderDefinition {
    std::filesystem::path header;
    std::string prefix;
    std::unordered_set<std::string> skipped{};
    HeaderFormat format{HeaderFormat::either};

    bool operator==(const ProviderDefinition &) const = default;

    /// @brief Builds the resolvable enum definition a header provider needs from this description.
    ///
    /// @details
    /// A ProviderDefinition describes a field's value names in the abstract; a provider that reads a header also
    /// needs the field's value cap and a human-readable field name for diagnostics. Those two facts live on
    /// the owning Field, not here, so the caller supplies them. The header path is intentionally not carried
    /// over: it is resolved separately against the project root before a provider is built.
    ///
    /// @param field_display_name The field name used in this provider's diagnostics
    /// @param max_value The largest value a resolved name may hold, from the field's width
    /// @return An EnumDefinition copying this definition's prefix, skip set, and format alongside the two given facts
    [[nodiscard]] EnumDefinition to_enum_definition(std::string field_display_name, std::uint32_t max_value) const;
};

/// @brief The self-contained description a header enum provider needs to scan and validate values.
///
/// @details
/// Where ProviderDefinition describes how a field's values are declared in the abstract, an EnumDefinition is the
/// concrete, resolvable form a provider consumes: it adds the field's value cap and a display name for
/// diagnostics, and drops the header path (resolved separately against the project root). It is plain data
/// derived from a ProviderDefinition plus its owning Field via ProviderDefinition::to_enum_definition.
struct EnumDefinition {
    std::string prefix;
    std::uint32_t max_value;
    std::unordered_set<std::string> skipped{};
    HeaderFormat format{HeaderFormat::either};
    std::string field_display_name;

    bool operator==(const EnumDefinition &) const = default;
};

/// @brief One named bit-field within a metatile attribute layout.
///
/// @details
/// A field pairs a name with the mask of bits it occupies inside the packed attribute word. The mask is
/// the single source of truth for the field's position and size: the offset, width, and maximum storable
/// value are all derived from it. A field may also carry a provider definition describing how its values are
/// named in a base-game header, and an optional role marking it as a field whose values Porytiles manages
/// (see FieldRole).
///
/// A Field is a passive carrier and does not validate itself. Layout rules (contiguous non-zero mask,
/// in-range, non-overlapping, default fits) are enforced when a Field is placed into a Schema via
/// Schema::create. Deriving offset() or max_value() from a zero mask is meaningless, so those accessors
/// document a non-zero-mask precondition that Schema::create guarantees for every field it accepts.
///
/// @invariant For a Field obtained from a created Schema, mask() is a single contiguous run of at least one bit.
class Field {
  public:
    /// @brief Constructs a field from a name, mask, and optional default value, provider definition, and role.
    ///
    /// @param name The field name
    /// @param mask The bits this field occupies within the packed attribute word
    /// @param default_value The value used when the field is absent from a metatile's attribute
    /// @param provider An optional description of how this field's named values are declared
    /// @param role An optional semantic role marking a field whose values Porytiles manages
    Field(
        std::string name,
        std::uint32_t mask,
        std::uint32_t default_value = 0,
        std::optional<ProviderDefinition> provider = std::nullopt,
        std::optional<FieldRole> role = std::nullopt)
        : name_{std::move(name)}, mask_{mask}, default_value_{default_value}, provider_{std::move(provider)},
          role_{role}
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

    /// @brief Returns the bit offset of the field's least-significant bit.
    ///
    /// @pre mask() must not be zero. Guaranteed for any field inside a created Schema.
    /// @return The number of low-order zero bits in the mask
    [[nodiscard]] std::uint32_t offset() const
    {
        return static_cast<std::uint32_t>(std::countr_zero(mask_));
    }

    /// @brief Returns the number of bits the field occupies.
    ///
    /// @return The population count of the mask
    [[nodiscard]] std::uint32_t width() const
    {
        return static_cast<std::uint32_t>(std::popcount(mask_));
    }

    /// @brief Returns the largest value the field can hold.
    ///
    /// @details
    /// This is the mask shifted down to bit zero, which is the single source of truth for the range checks
    /// that Schema::create and the provider-backed enum loading apply.
    ///
    /// @pre mask() must not be zero. Guaranteed for any field inside a created Schema.
    /// @return The maximum value representable in the field's width
    [[nodiscard]] std::uint32_t max_value() const
    {
        return mask_ >> offset();
    }

    [[nodiscard]] bool has_provider() const
    {
        return provider_.has_value();
    }

    [[nodiscard]] const std::optional<FieldRole> &role() const
    {
        return role_;
    }

    /// @brief Reports whether this field carries the layer_type role.
    ///
    /// @details
    /// The layer_type-role field receives each metatile's layer type when attributes are packed. Its
    /// per-metatile value comes from MetatileAttribute::layer_type() (inferred at compile time or pinned
    /// through the CSV's trailing layer_type column), never from a normal CSV value column.
    ///
    /// @return true when the field's role is FieldRole::layer_type
    [[nodiscard]] bool packs_layer_type() const
    {
        return role_ == FieldRole::layer_type;
    }

    /// @brief Returns the field's provider definition.
    ///
    /// @pre has_provider() must be true.
    /// @return A const reference to the provider definition
    [[nodiscard]] const ProviderDefinition &provider_definition() const
    {
        assert_or_panic(provider_.has_value(), "Field::provider_definition() called on a field with no provider");
        return provider_.value();
    }

  private:
    std::string name_;
    std::uint32_t mask_;
    std::uint32_t default_value_;
    std::optional<ProviderDefinition> provider_;
    std::optional<FieldRole> role_;
};

/// @brief A validated metatile attribute layout: an ordered set of non-overlapping fields.
///
/// @details
/// A Schema is the single source of truth for how a packed attribute word is laid out. It owns the
/// fields that make up an attribute word and the byte size those fields were validated against. Schemas
/// can only be built through Schema::create, which enforces the layout rules, so any Schema in hand is
/// known to be well-formed: every field has a contiguous non-zero mask that fits the attribute size, no
/// two fields overlap, no name repeats, and every default fits its field.
///
/// The layer type is an ordinary field carrying FieldRole::layer_type (see FieldRole). At most one field
/// may carry the role, and a schema without one has the layer type disabled: every metatile reads back as
/// LayerType::normal and no layer-type bits are packed. The role field's per-metatile value is managed by
/// Porytiles (compile-time inference or a CSV pin), so it is excluded from value_fields(), the field list
/// the attributes CSV columns are built from.
class Schema {
  public:
    /// @brief Validates a set of fields against an attribute size and builds a Schema.
    ///
    /// @details
    /// Runs a single fail-fast pass over the fields in the given order. For each field the duplicate-name
    /// check runs first, then the intra-field rules (zero mask, non-contiguous mask, mask beyond the
    /// attribute size, default value too large), then the layer_type-role rules (at most one role field;
    /// no provider or default on it; the name "layer_type" requires the role), then the cross-field
    /// overlap check against the fields already seen. The first violation wins and is returned as the
    /// error; on success the fields are stored in the order given.
    ///
    /// @param fields The fields making up the layout, in the order they should be preserved
    /// @param attribute_bytes The attribute size in bytes the layout is validated against
    /// @pre @p attribute_bytes must be 1, 2, or 4.
    /// @return A validated Schema, or an error describing the first layout rule violation
    [[nodiscard]] static ChainableResult<Schema> create(std::vector<Field> fields, std::size_t attribute_bytes);

    [[nodiscard]] const std::vector<Field> &fields() const
    {
        return fields_;
    }

    /// @brief Returns the fields that hold plain per-metatile values, excluding the layer_type-role field.
    ///
    /// @details
    /// This is the field list the attributes CSV is built from: one value column per entry, in schema
    /// order. The layer_type-role field never appears here because its values are managed by Porytiles
    /// (compile-time inference or a CSV pin through the separate trailing layer_type column), not entered
    /// as a value column. Code that renders, parses, or defaults per-field values should iterate this
    /// list; code that needs the full packed layout (binary pack/unpack, schema dumps) iterates fields().
    ///
    /// @return The non-role fields, in schema order
    [[nodiscard]] const std::vector<Field> &value_fields() const
    {
        return value_fields_;
    }

    [[nodiscard]] std::size_t attribute_bytes() const
    {
        return attribute_bytes_;
    }

    /// @brief Returns the field carrying the layer_type role, or nullptr when the layer type is disabled.
    ///
    /// @return A pointer to the role field within fields(), or nullptr when no field carries the role
    [[nodiscard]] const Field *layer_type_field() const
    {
        return layer_type_index_.has_value() ? &fields_[layer_type_index_.value()] : nullptr;
    }

    /// @brief Returns the mask of the layer_type bits within the packed attribute word.
    ///
    /// @details
    /// This is the mask of the field carrying FieldRole::layer_type. A returned value of 0 means no field
    /// carries the role, so the layer type is disabled: no bits are packed and every metatile decodes as
    /// LayerType::normal.
    ///
    /// @return The layer_type-role field's mask, or 0 when the layer type is disabled
    [[nodiscard]] std::uint32_t layer_type_mask() const
    {
        return layer_type_mask_;
    }

    /// @brief Returns the bit offset of the layer_type's least-significant bit.
    ///
    /// @details
    /// Meaningful only when layer_type_mask() is non-zero. For a disabled (zero) mask the offset is not
    /// used: the binary pack/unpack skips the layer type entirely.
    ///
    /// @return The number of low-order zero bits in layer_type_mask()
    [[nodiscard]] std::uint32_t layer_type_offset() const
    {
        return static_cast<std::uint32_t>(std::countr_zero(layer_type_mask_));
    }

  private:
    Schema(std::vector<Field> fields, std::size_t attribute_bytes, std::optional<std::size_t> layer_type_index)
        : fields_{std::move(fields)}, attribute_bytes_{attribute_bytes}, layer_type_index_{layer_type_index},
          layer_type_mask_{layer_type_index.has_value() ? fields_[layer_type_index.value()].mask() : 0}
    {
        for (const Field &field : fields_) {
            if (!field.packs_layer_type()) {
                value_fields_.push_back(field);
            }
        }
    }

    std::vector<Field> fields_;
    // The non-role fields, materialized once at creation so CSV-facing consumers cannot forget to skip the
    // layer_type-role field. Safe to duplicate: a Schema is immutable after creation.
    std::vector<Field> value_fields_;
    std::size_t attribute_bytes_;
    // Index rather than pointer so the cache survives Schema copies and moves.
    std::optional<std::size_t> layer_type_index_;
    std::uint32_t layer_type_mask_;
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

template <>
struct std::formatter<porytiles::FieldRole> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles::FieldRole &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles::to_string(value));
    }
};
