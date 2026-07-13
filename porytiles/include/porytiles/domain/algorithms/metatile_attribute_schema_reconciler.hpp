#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "gsl/pointers"

#include "porytiles/domain/algorithms/metatile_attribute_inference.hpp"
#include "porytiles/domain/config/metatile_attribute_field_spec.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

class TextFormatter;

/// @brief The product of reconciling the project's metatile attribute schema.
///
/// @details
/// @c schema holds the validated fields, sized against @c attribute_bytes. @c loaded_specs is the full post-merge
/// spec list, useful for diagnostics. @c attribute_bytes is the byte width the schema was validated against; it may
/// have been widened past an assumed width to cover the configured masks. @c declaration_bytes is the declared
/// element width for generated @c gMetatileAttributes_* C declarations (const uN / INCBIN_UN); it can differ from
/// @c attribute_bytes, matching pokeemerald-expansion's FRLG build, whose attribute arrays are declared 'const u16'
/// but read as 4-byte words. @c fields_origin, @c size_origin, and @c declaration_origin are human-readable notes on
/// where the field set, the attribute width, and the declaration width came from (an explicit config, an inferred
/// project fact, or an assumption).
struct LoadedMetatileAttributeSchema {
    Schema schema;
    MetatileAttributeFieldSpecs loaded_specs;
    std::size_t attribute_bytes;
    std::size_t declaration_bytes;
    std::string fields_origin;
    std::string size_origin;
    std::string declaration_origin;
};

/// @brief The severity of a non-fatal reconciliation note.
enum class AttributeNoteSeverity {
    remark,
    warning,
};

/// @brief One non-fatal note produced while reconciling the metatile attribute schema.
///
/// @details
/// Notes are returned as data rather than emitted to a sink so the caller decides where they go (the filtered user
/// diagnostics in the CLI) and so they can be asserted on directly in tests. @c text is fully formatted.
struct AttributeReconcileNote {
    AttributeNoteSeverity severity;
    std::string text;
};

/// @brief The user-stated config inputs to metatile attribute schema reconciliation.
///
/// @details
/// Every member reflects what the user actually said, never a derived fact: @c fields is the explicit
/// metatile_attribute_fields list (empty means not declared), @c attribute_size is the explicit width knob (nullopt
/// means the user did not pin the width), @c declaration_size and @c layer_type_mask are the matching override knobs.
/// The @c *_source strings are the config provenance descriptions used in origin text and errors, and @c scan_source
/// is the fieldmap header path the inference facts came from.
struct MetatileAttributeConfigInputs {
    MetatileAttributeFieldSpecs fields;
    std::string fields_source;
    MetatileAttributeFieldOverrides overrides;
    std::optional<std::size_t> attribute_size;
    std::string attribute_size_source;
    std::optional<std::size_t> declaration_size;
    std::optional<std::uint32_t> layer_type_mask;
    std::string scan_source;
};

/// @brief The product of a reconciliation run: the loaded schema (or a fatal error) plus the non-fatal notes.
///
/// @details
/// @c notes rides beside @c result rather than inside the success payload because the assumed-width warning must
/// survive onto the error path: it is decided before reconciliation can still fail, and the user should see it either
/// way. @c notes is valid on success and on failure, in emission order.
struct MetatileAttributeReconciliation {
    ChainableResult<LoadedMetatileAttributeSchema> result;
    std::vector<AttributeReconcileNote> notes;
};

/// @brief Reconciles the inferred metatile attribute facts with the user's config into a loaded schema.
///
/// @details
/// This is the single place where the project's inferred attribute facts meet the user's stated config. The decision
/// procedure:
///
/// 1. Width: an explicit @c attribute_size is authoritative. Otherwise exactly one non-synthesized candidate set
///    makes its required width authoritative; two or more are a fatal dual-layout error (no project file records
///    which build flavor the user targets); zero (including an invalid or empty inference) falls back to an assumed,
///    non-authoritative 2 bytes plus a warning note. A synthesized (stock behavior-only) set never drives the width:
///    its 2 bytes are an assumption, not a project fact.
/// 2. Fields: explicit @c fields are the truth, and selection over the candidate sets is advisory (it only supplies
///    the layer-type mask; an invalid inference or a non-unique width match simply leaves it null). Otherwise the
///    unique candidate set whose required width equals the resolved width is selected (with a remark note); an
///    invalid inference, no width match, or an ambiguous match is fatal, each error naming its escape hatch.
/// 3. Layer-type mask: the explicit knob when set, else the selected set's structural mask, else unset (the
///    size-based default applies in Schema::create). This is independent of where the fields came from.
/// 4. Merge and widen: overrides merge into the fields, and the masks may widen a non-authoritative width silently;
///    a mask exceeding an authoritative width is fatal.
/// 5. Declaration width: the explicit knob, else the width inferred from struct Tileset's declaration, else the
///    post-widening attribute width.
///
/// The loaded schema's origin strings (fields, size, declaration) are filled here. A final remark note summarizes
/// the resolved layout.
///
/// @param inference The inferred candidate sets and declaration width (default-constructed when no fieldmap present)
/// @param inputs The user's config inputs and their provenance descriptions
/// @param format The formatter used for note and error text
/// @return The reconciliation: the loaded schema or the first fatal error, plus the non-fatal notes
[[nodiscard]] MetatileAttributeReconciliation reconcile_metatile_attribute_schema(
    const MetatileAttributeInferenceResult &inference,
    const MetatileAttributeConfigInputs &inputs,
    gsl::not_null<const TextFormatter *> format);

} // namespace porytiles
