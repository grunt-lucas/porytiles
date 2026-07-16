#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles/domain/algorithms/metatile_attribute_inference.hpp"
#include "porytiles/domain/config/metatile_attribute_field_definition.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

class TextFormatter;
class UserDiagnostics;

/// @brief The diagnostic tag the reconciler emits its remarks and warnings under.
inline constexpr auto metatile_attr_schema_tag = "metatile-attribute-schema";

/// @brief The product of reconciling the project's metatile attribute schema.
///
/// @details
/// @c schema holds the validated fields, sized against @c attribute_bytes. @c loaded_definitions is the full post-merge
/// definition list, useful for diagnostics. @c attribute_bytes is the byte width the schema was validated against: the
/// explicit width knob when set, otherwise the smallest of 1, 2, or 4 bytes covering the resolved field masks. @c
/// declaration_bytes is the declared element width for generated @c gMetatileAttributes_* C declarations (const uN /
/// INCBIN_UN); it can differ from @c attribute_bytes, matching pokeemerald-expansion's FRLG build, whose attribute
/// arrays are declared 'const u16' but read as 4-byte words. @c fields_origin, @c size_origin, and
/// @c declaration_origin are human-readable notes on where the field set, the attribute width, and the declaration
/// width came from (an explicit config or an inferred project fact).
struct LoadedMetatileAttributeSchema {
    Schema schema;
    MetatileAttributeFieldDefinitions loaded_definitions;
    std::size_t attribute_bytes;
    std::size_t declaration_bytes;
    std::string fields_origin;
    std::string size_origin;
    std::string declaration_origin;
};

/// @brief The user-stated config inputs to metatile attribute schema reconciliation.
///
/// @details
/// Every member reflects what the user actually said, never a derived fact: @c fields is the explicit
/// metatile_attribute_fields list (empty means not declared), @c attribute_size is the explicit width knob (nullopt
/// means the user did not pin the width), and @c declaration_size is the matching declaration-width knob. The
/// @c *_source strings are the config provenance descriptions used in origin text and errors, and @c scan_source is
/// the fieldmap header path the inference facts came from.
struct MetatileAttributeConfigInputs {
    MetatileAttributeFieldDefinitions fields;
    std::string fields_source;
    MetatileAttributeFieldOverrides overrides;
    std::optional<std::size_t> attribute_size;
    std::string attribute_size_source;
    std::optional<std::size_t> declaration_size;
    std::string scan_source;
};

/// @brief Reconciles the inferred metatile attribute facts with the user's config into a loaded schema.
///
/// @details
/// This is the single place where the project's inferred attribute facts meet the user's stated config. The decision
/// procedure:
///
/// 1. Dual layout: two or more inferred candidate sets with no explicit @c attribute_size are fatal, even when the
///    fields are explicit. The source tree holds both build flavors (stock pokeemerald-expansion) and no project
///    file records which one the build uses, so the read stride cannot be determined.
/// 2. Fields: explicit @c fields are the truth and inference is never consulted for content; when a usable inferred
///    layout exists and disagrees with them (by name, mask, or role), a warning says so. Otherwise the layout
///    must select cleanly from inference: an invalid inference propagates its error, zero candidates are fatal
///    (nothing to infer from and nothing configured), one candidate is selected, and two or more (the width knob is
///    set here, per step 1) select by required width: a unique exact match wins, else a unique narrower fit wins,
///    and anything else is fatal. Each error names its escape hatch.
/// 3. Merge: overrides merge into the field list; every override must name an existing field, and a merged field
///    must end up with a mask. The layer type is an ordinary field carrying FieldRole::layer_type, subject to the
///    same override mechanics as any other field; a schema without a role field has the layer type disabled.
/// 4. Width: the explicit knob when set; a merged mask needing a wider word than the knob is fatal, and a knob wider
///    than both the merged masks and the scanned declaration width draws a warning. With no knob the width is
///    the smallest of 1, 2, or 4 bytes covering the merged masks, and a scanned struct Tileset declaration wider
///    than that is fatal (masks prove a minimum width, never the width itself, so the knob must disambiguate).
/// 5. Declaration width: the explicit knob, else the width inferred from struct Tileset's declaration, else the
///    resolved attribute width.
///
/// The loaded schema's origin strings (fields, size, declaration) are filled here. A final remark summarizes the
/// resolved layout.
///
/// Non-fatal findings are emitted to @c diag under the "metatile-attribute-schema" tag at the point each one is
/// decided, so anything decided before a later fatal still reaches the user.
///
/// @param inference The inferred candidate sets and declaration width (default-constructed when no fieldmap present)
/// @param inputs The user's config inputs and their provenance descriptions
/// @param format The formatter used for note and error text
/// @param diag The sink the non-fatal remarks and warnings are emitted to
[[nodiscard]] ChainableResult<LoadedMetatileAttributeSchema> reconcile_metatile_attribute_schema(
    const MetatileAttributeInferenceResult &inference,
    const MetatileAttributeConfigInputs &inputs,
    gsl::not_null<const TextFormatter *> format,
    gsl::not_null<const UserDiagnostics *> diag);

} // namespace porytiles
