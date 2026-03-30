#pragma once

#include <cstddef>
#include <map>
#include <vector>

#include "porytiles2/domain/models/shape_mask.hpp"
#include "porytiles2/domain/models/shape_tile.hpp"

namespace porytiles2 {

/**
 * @brief A member of a ShapeGroup, representing one tile that shares a canonical shape with other members.
 *
 * @details
 * Each member records which input tile it corresponds to (via tile_index), its color assignment (the full
 * ShapeMask → PixelType mapping from its ShapeTile), and the flip flags needed to transform the canonical
 * shape back to this member's original orientation.
 *
 * @tparam PixelType The pixel type stored for each shape region (e.g., Rgba32)
 */
template <typename PixelType>
struct ShapeGroupMember {
    std::size_t tile_index;

    std::map<ShapeMask, PixelType> colors;

    bool h_flip;

    bool v_flip;
};

/**
 * @brief A group of tiles that share the same canonical shape but have different color assignments.
 *
 * @details
 * ShapeGroup captures the core insight for tile sharing: tiles with identical geometry (same ShapeMask layout after
 * canonicalization) but different color fills are candidates for palette sharing. If two such tiles land in different
 * palettes, aligning the palette slot indices for corresponding colors allows a single indexed tile to render with
 * either palette.
 *
 * A ShapeGroup with only one member has no sharing opportunity. A group with 2+ members whose colors differ across
 * members are sharing candidates, provided they end up in different palettes after packing.
 *
 * @tparam PixelType The pixel type stored for each shape region (e.g., Rgba32)
 */
template <typename PixelType>
struct ShapeGroup {
    ShapeTile<PixelType> canonical_shape;

    std::vector<ShapeGroupMember<PixelType>> members;
};

} // namespace porytiles2
