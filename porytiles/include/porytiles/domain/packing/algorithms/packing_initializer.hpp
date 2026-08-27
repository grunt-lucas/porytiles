#pragma once

#include <cstddef>
#include <set>
#include <vector>

#include "porytiles/domain/packing/models/packed_palette.hpp"
#include "porytiles/domain/packing/models/palette_pool.hpp"
#include "porytiles/domain/packing/models/prefilled_palette.hpp"

namespace porytiles {

/// @brief Initializes packed palettes from prefilled palettes.
///
/// @details
/// This function sets up the initial palette state for packing algorithms by creating PackedPalette objects for each
/// prefilled palette that's available in the pool. For each prefilled palette:
/// - Checks out the corresponding slot from the palette pool
/// - Calculates effective capacity accounting for "wasted" slots from duplicate colors
/// - Pre-populates with fixed colors via a "system" PackableTile
///
/// After this function returns, the caller can use the modified palette pool for strategy-specific initialization
/// (e.g., creating empty palettes for remaining slots).
///
/// @param prefilled_palettes The prefilled palettes to initialize from
/// @param palette_pool The palette pool; available slots for prefilled palettes will be checked out
/// @param palette_capacity The base capacity for each palette (typically 15 for GBA hardware)
/// @return Vector of initialized PackedPalette objects for the prefilled palettes
[[nodiscard]] std::vector<PackedPalette> initialize_packed_palettes(
    const std::set<PrefilledPalette> &prefilled_palettes, PalettePool &palette_pool, std::size_t palette_capacity);

} // namespace porytiles
