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
    /**
     * @brief Index of this tile in the input tile collection.
     */
    std::size_t tile_index;

    /**
     * @brief The color mapping for this member: ShapeMask → PixelType.
     *
     * @details
     * The keys (ShapeMasks) are in canonical orientation. Each member in the same ShapeGroup has the same set
     * of mask keys but potentially different PixelType values.
     */
    std::map<ShapeMask, PixelType> colors;

    /**
     * @brief Horizontal flip flag: true if flipping the canonical form horizontally reproduces this member's original.
     */
    bool h_flip;

    /**
     * @brief Vertical flip flag: true if flipping the canonical form vertically reproduces this member's original.
     */
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
 * members are sharing candidates — provided they end up in different palettes after packing.
 *
 * @tparam PixelType The pixel type stored for each shape region (e.g., Rgba32)
 */
template <typename PixelType>
struct ShapeGroup {
    /**
     * @brief The canonical shape key for this group.
     *
     * @details
     * This is the ShapeTile in canonical orientation (lexicographically minimal among all flip variants). All members
     * share this same shape structure; only their PixelType values differ.
     */
    ShapeTile<PixelType> canonical_shape;

    /**
     * @brief The members of this shape group.
     *
     * @details
     * Each member has the same canonical shape but potentially different color assignments. Members with identical
     * color assignments are deduplicated (they represent the exact same tile and don't contribute to sharing).
     */
    std::vector<ShapeGroupMember<PixelType>> members;
};

} // namespace porytiles2
