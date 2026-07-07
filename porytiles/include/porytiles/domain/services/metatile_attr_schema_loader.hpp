#pragma once

#include <cstddef>
#include <string>

#include "gsl/pointers"

#include "porytiles/domain/config/metatile_attr_field_spec.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

class TextFormatter;

/**
 * @brief The product of loading a metatile attribute schema from field specs and overrides.
 *
 * @details
 * @c schema is the validated primary-layout schema built from the fields that carry a primary mask. @c resolved_specs
 * is the full post-merge spec list, including alternate-layout-only fields (a frlg_mask but no primary mask) that are
 * intentionally absent from @c schema. Downstream per-tileset layout selection (issue #283) consumes the resolved
 * specs.
 */
struct LoadedAttrSchema {
    Schema schema;
    MetatileAttrFieldSpecs resolved_specs;
};

/**
 * @brief Merges field overrides into a baseline field list and builds a validated attribute schema.
 *
 * @details
 * Overrides are applied additively: each present override member replaces the baseline value, absent members fall
 * through, a present @c skipped set replaces the baseline skip set wholesale, and @c provider removal drops a field's
 * provider. Fields that carry a primary mask are validated into a Schema against @p attr_bytes; alternate-only fields
 * are retained in the resolved specs but excluded from the Schema.
 *
 * Hard errors: an empty field list; a baseline name defined more than once; an override naming a field that does not
 * exist; a provider override that leaves a field without both a header and a prefix; and a spec that ends up with
 * neither a mask nor a frlg_mask.
 *
 * @param fields The baseline field specs, in display order
 * @param overrides The per-field overrides to merge in
 * @param attr_bytes The attribute byte size the primary masks are validated against (2 or 4)
 * @param format The formatter used for diagnostic text
 * @return The loaded schema and resolved specs, or the first hard error encountered
 */
[[nodiscard]] ChainableResult<LoadedAttrSchema> load_metatile_attr_schema(
    const MetatileAttrFieldSpecs &fields,
    const MetatileAttrFieldOverrides &overrides,
    std::size_t attr_bytes,
    gsl::not_null<const TextFormatter *> format);

/**
 * @brief Which per-tileset attribute layout to resolve: the primary (Ruby/Sapphire/Emerald) or FRLG alternate.
 */
enum class AttrSchemaLayout { primary, frlg };

[[nodiscard]] inline std::string to_string(AttrSchemaLayout layout)
{
    switch (layout) {
    case AttrSchemaLayout::primary:
        return "primary";
    case AttrSchemaLayout::frlg:
        return "frlg";
    }
    return "primary";
}

/**
 * @brief The product of resolving a per-tileset attribute schema for a chosen layout.
 *
 * @details
 * @c schema holds the fields selected for @c layout (the primary mask for @c primary, the frlg_mask for @c frlg),
 * validated against @c attr_bytes. @c resolved_specs is the full post-merge spec list (both layouts' masks), useful for
 * diagnostics and for reporting which fields were excluded for the chosen layout. @c attr_bytes is the byte width the
 * schema was validated against, which may have been widened past the detected size to cover the selected masks.
 */
struct ResolvedTilesetAttrSchema {
    Schema schema;
    MetatileAttrFieldSpecs resolved_specs;
    AttrSchemaLayout layout;
    std::size_t attr_bytes;
};

/**
 * @brief Resolves the attribute schema for one tileset under a chosen layout, widening the attr size as needed.
 *
 * @details
 * Merges @p overrides into @p fields (same rules as load_metatile_attr_schema), then selects, per spec, the primary
 * @c mask when @p layout is @c primary or the @c frlg_mask when @p layout is @c frlg. Specs lacking the selected mask
 * are excluded symmetrically. If no field survives selection, a semantic error is returned naming the fix (add a mask
 * for the layout, or switch use_frlg_alternate_masks).
 *
 * The attribute byte width is widened silently to the smallest of 2 or 4 bytes that covers the selected masks, but
 * never below @p detected_attr_bytes (the width detected from the project's own metatiles.h declarations).
 *
 * @param fields The baseline field specs, in display order
 * @param overrides The per-field overrides to merge in
 * @param layout The layout to resolve (primary or frlg)
 * @param detected_attr_bytes The attribute byte size detected from the project (2 or 4)
 * @param format The formatter used for diagnostic text
 * @return The resolved schema, specs, layout, and final byte width, or the first hard error encountered
 */
[[nodiscard]] ChainableResult<ResolvedTilesetAttrSchema> resolve_tileset_attr_schema(
    const MetatileAttrFieldSpecs &fields,
    const MetatileAttrFieldOverrides &overrides,
    AttrSchemaLayout layout,
    std::size_t detected_attr_bytes,
    gsl::not_null<const TextFormatter *> format);

} // namespace porytiles
