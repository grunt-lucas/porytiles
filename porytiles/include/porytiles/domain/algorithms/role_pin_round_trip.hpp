#pragma once

#include <optional>

#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/models/porytiles_tileset_component.hpp"
#include "porytiles/utilities/panic/panic.hpp"

namespace porytiles {

/// @brief Applies the decompile/import round-trip rules to one metatile's layer_type pin.
///
/// @details
/// The decompiler rebuilds each metatile's attribute from metatile_attributes.bin (unpinned), then calls this to decide
/// whether the rebuilt attribute should carry an explicit layer_type pin. The decision is made purely by looking at the
/// data (the prior CSV state and the prior per-row attribute), never by user config. The rules:
///
/// - no_csv / column_absent: pin every row from `metatile_attributes.bin`. This applies to a fresh import, a decompile
/// with no attributes.csv, and a decompile whose CSV lacked the active pin column.
///
/// - column_present: preserve each row's prior pin state. A prior row that carried a pin stays pinned, with its value
/// refreshed from the relevant contents in `metatile_attributes.bin`; a blank cell or a row absent from the prior
/// CSV stays unpinned to be filled in by inference.
///
/// The pinned value always comes from the fresh, metatile_attributes.bin layer type; the prior pin only decides
/// *whether* the row stays pinned, not what value it takes.
///
/// @param fresh_from_bin The attribute freshly decoded from metatile_attributes.bin (its layer_type is the bin value)
/// @param prior_state What the loaded attributes.csv (if any) said about the layer_type pin column
/// @param prior_attribute The prior per-row attribute from the loaded CSV, or nullopt when the row was absent
/// @return The attribute with its layer_type pin set (or left unpinned) per the round-trip rules
[[nodiscard]] inline MetatileAttribute merge_prior_layer_type_pin(
    MetatileAttribute fresh_from_bin,
    PriorPinColumnState prior_state,
    const std::optional<MetatileAttribute> &prior_attribute)
{
    switch (prior_state) {
    case PriorPinColumnState::no_csv:
    case PriorPinColumnState::column_absent:
        fresh_from_bin.explicit_layer_type(fresh_from_bin.layer_type());
        return fresh_from_bin;
    case PriorPinColumnState::column_present:
        if (prior_attribute.has_value() && prior_attribute->explicit_layer_type().has_value()) {
            fresh_from_bin.explicit_layer_type(fresh_from_bin.layer_type());
        }
        return fresh_from_bin;
    }
    panic("merge_prior_layer_type_pin: unhandled PriorPinColumnState value");
}

} // namespace porytiles
