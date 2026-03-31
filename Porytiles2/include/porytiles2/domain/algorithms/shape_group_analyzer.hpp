#pragma once

#include <cstddef>
#include <map>
#include <vector>

#include "porytiles2/domain/models/canonical_shape_tile.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/shape_group.hpp"
#include "porytiles2/domain/models/shape_mask.hpp"
#include "porytiles2/domain/models/shape_tile.hpp"
#include "porytiles2/domain/models/supports_transparency.hpp"

namespace porytiles2 {

/**
 * @brief Converts a PixelTile to a ShapeTile by grouping pixels by color into ShapeMasks.
 *
 * @details
 * For each unique non-transparent color in the tile, creates a ShapeMask marking which pixels have that color,
 * then maps the mask to the color value. This produces a ShapeTile where the keys (ShapeMasks) represent the
 * geometric structure and the values (PixelType) represent the color assignments.
 *
 * This is a direct conversion that does not require a ColorIndexMap, unlike the from_pixel_tile() functions
 * in tile_converters.hpp which produce ShapeTile<ColorIndex>.
 *
 * @tparam PixelType The pixel type, must support transparency checking
 * @param pixel_tile The input pixel tile to convert
 * @param is_transparent_pred Predicate that returns true if a pixel is transparent
 * @return A ShapeTile with color-to-mask mappings
 */
template <SupportsTransparency PixelType, typename TransparencyPredicate>
[[nodiscard]] ShapeTile<PixelType>
shape_tile_from_pixel_tile(const PixelTile<PixelType> &pixel_tile, TransparencyPredicate is_transparent_pred)
{
    std::map<PixelType, ShapeMask> color_to_mask;

    for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
        for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
            const auto pixel = pixel_tile.at(row, col);
            if (is_transparent_pred(pixel)) {
                continue;
            }
            color_to_mask[pixel].set(row, col);
        }
    }

    ShapeTile<PixelType> result;
    for (const auto &[color, mask] : color_to_mask) {
        result.set(mask, color);
    }
    return result;
}

/**
 * @brief Analyzes a collection of pixel tiles and groups them by canonical shape for tile sharing analysis.
 *
 * @details
 * This algorithm detects color-isomorphic tiles, i.e. tiles that share the same geometric structure (ShapeMask layout)
 * but have different color assignments. These are candidates for tile sharing via palette slot alignment.
 *
 * The shape-based grouping approach is inspired by borytiles by ishax-kos
 * (https://github.com/ishax-kos/borytiles), specifically its @c tileset.rs module. Borytiles represents tiles as
 * @c Shape_indexable_tile (a @c BTreeMap<Tile_mask,Color_index>), groups them by @c Shape (the set of
 * @c Tile_mask keys), and canonicalizes via @c get_ideal_flip (lexicographically minimal flip variant). Porytiles2
 * adapts these concepts into @c ShapeTile, @c ShapeMask, and @c CanonicalShapeTile respectively.
 *
 * The algorithm:
 * 1. For each tile, creates a ShapeTile<PixelType> directly from the pixel data
 * 2. Canonicalizes each ShapeTile (finds lexicographically minimal flip variant via shape-only comparison)
 * 3. Groups tiles by canonical shape using shape-only comparison (ignoring color values)
 * 4. Within each group, collects members with their color mappings and flip flags
 * 5. Returns only groups with 2+ members that have distinct color assignments
 *
 * Groups where all members have identical colors are excluded. They represent exact duplicates, not sharing
 * candidates.
 *
 * @tparam PixelType The pixel type, must support extrinsic transparency
 * @param tiles The input pixel tiles to analyze
 * @param extrinsic The extrinsic transparency color
 * @return Vector of ShapeGroups, each containing 2+ members with distinct colors sharing the same canonical shape
 */
template <SupportsTransparency PixelType>
[[nodiscard]] std::vector<ShapeGroup<PixelType>>
analyze_shape_groups(const std::vector<PixelTile<PixelType>> &tiles, const PixelType &extrinsic)
    requires requires(const PixelType &c) { c.is_transparent(c); }
{
    /*
     * Use a comparator that compares ShapeTiles by shape only (ignoring pixel values). This groups tiles with
     * the same geometric structure regardless of their color assignments.
     */
    auto shape_only_less = [](const ShapeTile<PixelType> &a, const ShapeTile<PixelType> &b) {
        return ShapeTile<PixelType>::compare_shape_only(a, b);
    };

    // Map canonical shape (by geometry only) -> list of members
    std::map<ShapeTile<PixelType>, std::vector<ShapeGroupMember<PixelType>>, decltype(shape_only_less)> groups(
        shape_only_less);

    for (std::size_t i = 0; i < tiles.size(); ++i) {
        auto shape_tile = shape_tile_from_pixel_tile(
            tiles[i], [&extrinsic](const PixelType &c) { return c.is_transparent(extrinsic); });

        // Skip fully transparent tiles
        if (shape_tile.is_transparent()) {
            continue;
        }

        CanonicalShapeTile<PixelType> canonical{shape_tile};

        ShapeGroupMember<PixelType> member;
        member.tile_index = i;
        member.colors = canonical.colors();
        member.h_flip = canonical.h_flip();
        member.v_flip = canonical.v_flip();

        // Use the canonical form (as ShapeTile) as the grouping key
        groups[static_cast<const ShapeTile<PixelType> &>(canonical)].push_back(std::move(member));
    }

    // Filter to groups with 2+ members that have distinct color assignments
    std::vector<ShapeGroup<PixelType>> result;
    for (auto &[canonical_shape, members] : groups) {
        if (members.size() < 2) {
            continue;
        }

        // Check if there are at least 2 distinct color assignments
        bool has_distinct_colors = false;
        for (std::size_t j = 1; j < members.size(); ++j) {
            if (members[j].colors != members[0].colors) {
                has_distinct_colors = true;
                break;
            }
        }

        if (!has_distinct_colors) {
            continue;
        }

        ShapeGroup<PixelType> group;
        group.canonical_shape = canonical_shape;
        group.members = std::move(members);
        result.push_back(std::move(group));
    }

    return result;
}

} // namespace porytiles2
