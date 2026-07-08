#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "gsl/pointers"

#include "porytiles/domain/config/metatile_attr_field_spec.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

class TextFormatter;

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
 * schema was validated against: for the primary layout it may have been widened past the detected size to cover the
 * selected masks, and for the frlg layout it is always 4 (the engine reads FRLG-layout attributes through a hardcoded
 * 'const u32 *' cast). @c declaration_bytes is the width for generated @c gMetatileAttributes_* C declarations: the
 * authoritative declared width when one exists, else @c attr_bytes. It may be narrower than @c attr_bytes for frlg,
 * matching pokeemerald-expansion, whose FRLG tilesets are declared 'const u16' but read as 4-byte words.
 */
struct ResolvedTilesetAttrSchema {
    Schema schema;
    MetatileAttrFieldSpecs resolved_specs;
    AttrSchemaLayout layout;
    std::size_t attr_bytes;
    std::size_t declaration_bytes;
};

/**
 * @brief Resolves the attribute schema for one tileset under a chosen layout, widening the attr size as needed.
 *
 * @details
 * Overrides are applied additively (each present override member replaces the baseline value, a present @c skipped set
 * replaces the baseline skip set wholesale, and @c provider removal drops a field's provider), rejecting an empty field
 * list, a duplicate baseline name, an override naming an unknown field, a provider override that leaves a field without
 * both a header and a prefix, and a spec with neither a mask nor a frlg_mask. It then selects, per spec, the primary
 * @c mask when @p layout is @c primary or the @c frlg_mask when @p layout is @c frlg. Specs lacking the selected mask
 * are excluded symmetrically. If no field survives selection, a semantic error is returned naming the fix (add a mask
 * for the layout, or switch use_frlg_alternate_masks).
 *
 * How the attribute byte width is decided depends on @p layout. The frlg layout always resolves 4 bytes: the engine
 * reads FRLG-layout attributes through a hardcoded 'const u32 *' cast, so the entry width is fixed regardless of what
 * metatiles.h declares. For the primary layout the width depends on @p detected_width_is_authoritative. When it is
 * @c false the detected width was only a guessed default (the project has no metatiles.h declaration to read), so the
 * selected masks are the sole evidence of the true width and the word is widened silently to the smallest of 1, 2, or
 * 4 bytes that covers them, never below @p detected_attr_bytes. When it is @c true the detected width came from a real
 * @c const @c uN declaration and is fixed by the base game (shared across every tileset), so a mask that needs a wider
 * word contradicts a hard fact and is reported as a hard error rather than silently widening.
 *
 * @param fields The baseline field specs, in display order
 * @param overrides The per-field overrides to merge in
 * @param layout The layout to resolve (primary or frlg)
 * @param detected_attr_bytes The attribute byte size detected from the project (1, 2, or 4); does not constrain the
 *        frlg width, but an authoritative value still becomes @c declaration_bytes
 * @param detected_width_is_authoritative Whether @p detected_attr_bytes came from a real declaration (true) or is a
 *        guessed default (false); for the primary layout, true forbids widening past it and false allows silent
 *        widening to fit the masks. Does not constrain the frlg width, but feeds @c declaration_bytes
 * @param layer_type_mask The resolved layer_type mask (0 disables it), or nullopt to use the size-based default
 * @param format The formatter used for diagnostic text
 * @return The resolved schema, specs, layout, final byte width, and declaration width, or the first hard error
 *         encountered
 */
[[nodiscard]] ChainableResult<ResolvedTilesetAttrSchema> resolve_tileset_attr_schema(
    const MetatileAttrFieldSpecs &fields,
    const MetatileAttrFieldOverrides &overrides,
    AttrSchemaLayout layout,
    std::size_t detected_attr_bytes,
    bool detected_width_is_authoritative,
    std::optional<std::uint32_t> layer_type_mask,
    gsl::not_null<const TextFormatter *> format);

} // namespace porytiles
