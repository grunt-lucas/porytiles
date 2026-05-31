#include "porytiles/domain/packing/algorithms/sharing_metrics.hpp"

#include <algorithm>

namespace porytiles {

bool palette_contains_sibling(
    const PackableTile::Id &tile_id, const PackedPalette &palette, const ShapeGroupMetadata &metadata)
{
    auto it = metadata.tile_id_to_group.find(tile_id);
    if (it == metadata.tile_id_to_group.end()) {
        return false;
    }

    std::size_t group_index = it->second;
    const auto &members = metadata.group_members[group_index];

    for (const auto &assigned_id : palette.assigned_tile_ids()) {
        if (assigned_id == tile_id) {
            continue;
        }
        if (std::ranges::find(members, assigned_id) != members.end()) {
            return true;
        }
    }

    return false;
}

double compute_sharing_penalty(
    const PackableTile &tile, const PackedPalette &palette, const ShapeGroupMetadata &metadata, double sharing_weight)
{
    if (!palette_contains_sibling(tile.id(), palette, metadata)) {
        return 0.0;
    }
    return sharing_weight * static_cast<double>(tile.color_count());
}

} // namespace porytiles
