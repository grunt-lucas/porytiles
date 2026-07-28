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
    Schema schema;                                        ///< The validated fields.
    MetatileAttributeFieldDefinitions loaded_definitions; ///< The post-merge definition list, useful for diagnostics.
    std::size_t attribute_bytes;                          ///< The actual attribute width.
    std::size_t declaration_bytes;  ///< The byte declaration width for gMetatileAttributes_* C decls.
    std::string fields_origin;      ///< Human-readable provenance notes.
    std::string size_origin;        ///< Human-readable provenance notes.
    std::string declaration_origin; ///< Human-readable provenance notes.
};

/// @brief The user-stated config inputs to metatile attribute schema reconciliation.
struct MetatileAttributeConfigInputs {
    MetatileAttributeFieldDefinitions fields;
    std::string fields_source;
    MetatileAttributeFieldOverrides overrides;
    std::optional<std::size_t> attribute_size;
    std::string attribute_size_source;
    std::optional<std::size_t> declaration_size;
    std::string fieldmap_header_source;
};

/// @brief Reconciles the inferred metatile attribute facts with the user's config, returning a
/// `LoadedMetatileAttributeSchema`.
///
/// @details
/// This function is responsible for comparing the project's inferred attribute facts with the user's stated config, and
/// deciding what, if any, config should be returned. The decision procedure:
///
/// The loaded schema contains origin strings (fields, size, declaration) to point at value sources for provenance
/// output. A final remark summarizes the resolved layout.
///
/// Non-fatal findings are emitted to `diag` under the "metatile-attribute-schema" tag at the point each one is
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
