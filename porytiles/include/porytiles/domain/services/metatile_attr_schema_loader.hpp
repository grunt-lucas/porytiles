#pragma once

#include <cstddef>

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

} // namespace porytiles
