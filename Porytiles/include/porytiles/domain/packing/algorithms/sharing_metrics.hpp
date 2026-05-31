#pragma once

#include "porytiles/domain/packing/models/packable_tile.hpp"
#include "porytiles/domain/packing/models/packed_palette.hpp"
#include "porytiles/domain/packing/models/shape_group_metadata.hpp"

namespace porytiles {

/**
 * @brief Checks whether a palette already contains a sibling of the given tile (same shape group, different tile).
 *
 * @details
 * Looks up the tile's PackableTile::Id in the metadata to find its shape group, then scans the palette's assigned tile
 * IDs for any other tile belonging to the same group. This is an O(G * N) check where G is the group size (typically
 * 2-5) and N is the number of tiles in the palette.
 *
 * @param tile_id The ID of the tile being placed.
 * @param palette The candidate palette to check.
 * @param metadata The shape group metadata mapping tile IDs to groups.
 * @return true if the palette contains at least one sibling from the same shape group.
 */
[[nodiscard]] bool palette_contains_sibling(
    const PackableTile::Id &tile_id, const PackedPalette &palette, const ShapeGroupMetadata &metadata);

/**
 * @brief Computes the sharing penalty for placing a tile in a palette.
 *
 * @details
 * Returns 0.0 if the tile is not in a shape group or no sibling is present in the palette. Otherwise returns
 * @p sharing_weight multiplied by the tile's color count. This penalty is added to the base weighted cost to
 * deprioritize palettes that already contain a sibling, steering shape group members toward different palettes.
 *
 * @param tile The tile being placed.
 * @param palette The candidate palette.
 * @param metadata The shape group metadata.
 * @param sharing_weight The penalty multiplier (default 0.5).
 * @return The sharing penalty to add to the base cost.
 */
[[nodiscard]] double compute_sharing_penalty(
    const PackableTile &tile,
    const PackedPalette &palette,
    const ShapeGroupMetadata &metadata,
    double sharing_weight = 0.5);

} // namespace porytiles
