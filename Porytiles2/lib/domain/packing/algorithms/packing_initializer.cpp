#include "porytiles2/domain/packing/algorithms/packing_initializer.hpp"

#include "porytiles2/domain/packing/models/packable_tile.hpp"

namespace porytiles2 {

std::vector<PackedPalette> initialize_packed_palettes(
    const std::set<PrefilledPalette> &prefilled_pals, PalettePool &pal_pool, std::size_t pal_capacity)
{
    std::vector<PackedPalette> palettes;

    for (const auto &prefilled_pal : prefilled_pals) {
        // Only set up prefilled palettes for slots that were requested via PalettePool
        if (pal_pool.is_available(prefilled_pal.hardware_index())) {
            pal_pool.checkout(prefilled_pal.hardware_index());

            // Calculate effective capacity accounting for "wasted" slots from duplicate colors.
            // If a prefilled palette has 15 occupied slots but only 14 unique colors, there's 1 wasted slot.
            const ColorSet fixed_colors = prefilled_pal.fixed_colors();
            const std::size_t unique_color_count = color_set_count(fixed_colors);
            const std::size_t occupied_slot_count = prefilled_pal.occupied_slots();
            const std::size_t wasted_slot_count = occupied_slot_count - unique_color_count;
            const std::size_t effective_capacity = pal_capacity - wasted_slot_count;

            PackedPalette pal{prefilled_pal.hardware_index(), effective_capacity};
            if (unique_color_count > 0) {
                // Pre-populate with fixed colors using a "system" tile whose id matches the prefilled's hw index
                PackableTile system_tile{
                    PackableTile::PrefilledPaletteId{prefilled_pal.hardware_index()}, fixed_colors};
                pal.add_tile(system_tile);
            }
            palettes.push_back(std::move(pal));
        }
    }

    return palettes;
}

} // namespace porytiles2
