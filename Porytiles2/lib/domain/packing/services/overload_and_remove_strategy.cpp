#include "porytiles2/domain/packing/services/overload_and_remove_strategy.hpp"

#include "porytiles2/domain/packing/algorithms/multiplicity_map_builder.hpp"
#include "porytiles2/domain/packing/algorithms/packing_initializer.hpp"
#include "porytiles2/domain/packing/models/packable_tile.hpp"

namespace porytiles2 {

ChainableResult<PackingOutput> OverloadAndRemoveStrategy::pack(const PackingInput &input) const
{
    PackingOutput output;
    PalettePool pal_pool = input.pal_pool_;

    // Initialize output palettes from prefilled palettes
    output.pals_ = initialize_packed_palettes(input.prefilled_pals_, pal_pool, input.pal_capacity_);

    // Ensure we have at least one palette
    if (output.pals_.empty()) {
        output.pals_.emplace_back(0, input.pal_capacity_);
    }

    // Build multiplicity map
    auto multiplicity = build_multiplicity_map(input.tiles_, input.hints_);

    // TODO: impl the rest

    return output;
}

} // namespace porytiles2
