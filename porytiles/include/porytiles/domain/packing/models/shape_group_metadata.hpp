#pragma once

#include <cstddef>
#include <map>
#include <vector>

#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/shape_group.hpp"
#include "porytiles/domain/packing/models/packable_tile.hpp"

namespace porytiles {

/// @brief Metadata that maps PackableTile IDs to shape group membership for sharing-aware packing.
///
/// @details
/// ShapeGroupMetadata is built from the output of @c analyze_shape_groups() and a parallel ID vector that maps combined
/// tile indices back to PackableTile::Id values. Strategies use this metadata to detect when a candidate palette
/// already contains a sibling tile from the same shape group, enabling sharing-aware cost penalties.
///
/// This struct is intended to be stored as an optional field in PackingInput. When absent, strategies behave
/// identically to their non-sharing-aware versions.
struct ShapeGroupMetadata {
    /// @brief Maps PackableTile::Id to its shape group index.
    ///
    /// @details
    /// Currently only RegularId tiles participate in shape groups. Anim tiles are excluded because they occupy
    /// DMA-overwritten VRAM slots and cannot share with regular tiles. Multi-palette animation support will re-enter
    /// anim tiles via user-provided color variants in a future feature. Only tiles that belong to a shape group with 2+
    /// members (i.e., tiles that have sharing potential) have entries in this map.
    std::map<PackableTile::Id, std::size_t> tile_id_to_group;

    /// @brief For each group, the list of PackableTile::Ids that belong to it.
    ///
    /// @details
    /// Indexed by group index. Each inner vector has 2+ entries (groups with fewer than 2 members are not included).
    std::vector<std::vector<PackableTile::Id>> group_members;
};

/// @brief Builds ShapeGroupMetadata from shape groups and a combined-index-to-ID mapping.
///
/// @details
/// The @p combined_index_to_id vector maps each index in the combined tile vector (used by @c analyze_shape_groups())
/// back to its corresponding PackableTile::Id. This allows shape group membership to be expressed in terms of
/// PackableTile::Id values, which strategies use for tile identification during packing.
///
/// @param shape_groups The shape groups produced by @c analyze_shape_groups().
/// @param combined_index_to_id Parallel vector mapping combined tile indices to PackableTile::Id values.
/// @pre Every @c member.tile_index in @p shape_groups must be a valid index into @p combined_index_to_id.
/// @return ShapeGroupMetadata with tile-to-group and group-to-members mappings.
[[nodiscard]] ShapeGroupMetadata build_shape_group_metadata(
    const std::vector<ShapeGroup<Rgba32>> &shape_groups, const std::vector<PackableTile::Id> &combined_index_to_id);

} // namespace porytiles
