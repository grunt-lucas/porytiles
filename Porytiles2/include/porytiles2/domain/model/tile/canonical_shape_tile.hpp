#pragma once

#include <algorithm>
#include <array>
#include <compare>
#include <vector>

#include "porytiles2/domain/model/tile/shape_tile.hpp"

namespace porytiles2 {

/**
 * @brief A ShapeTile representation that stores the canonical (lexicographically minimal) orientation among all flipped
 * variants.
 *
 * @details
 * CanonicalShapeTile finds and stores the canonical orientation of a tile by evaluating all four possible flip
 * combinations (no flip, horizontal only, vertical only, and both horizontal and vertical). The canonical orientation
 * is defined as the lexicographically minimal tile shape among these four variants.
 *
 * This representation enables:
 * - Shape-based tile deduplication: can detect when two tiles are isomorphic under flip or color transformations
 * - Canonical shape comparison: tiles can be compared in a normalized form regardless of their original orientation
 * - Orientation tracking: the flip flags record how to transform the canonical form back to the original orientation
 *
 * The stored flip flags (h_flip_ and v_flip_) represent the transformation needed to convert the canonical tile back
 * to the original input tile. For example, if the input tile's horizontal flip is its canonical form, h_flip() will
 * return true, indicating that horizontally flipping the canonical tile reproduces the original.
 *
 * Key difference from CanonicalPixelTile:
 * - Uses ShapeTile's custom operator< which compares ONLY shape masks (geometry), not pixel values (colors)
 * - This means tiles with the same shape but different color assignments will canonicalize to the same shape structure
 * - This is critical for shape-based tile deduplication that is color-agnostic
 *
 * This approach is particularly useful for:
 * - Tileset generation where tiles should be deduplicated based on shape regardless of colors
 * - Hardware targeting that supports tile flipping (e.g., GBA hardware attributes)
 * - Optimizing tileset size by identifying tiles that are rotations/flips of each other by shape
 * - Palette-independent tile analysis and optimization
 *
 * @tparam PixelType The pixel type stored for each shape region
 */
template <typename PixelType>
class CanonicalShapeTile : public ShapeTile<PixelType> {
  public:
    virtual ~CanonicalShapeTile() = default;

    /**
     * @brief Constructs a CanonicalShapeTile by finding the canonical orientation of the input tile.
     *
     * @details
     * This constructor evaluates all four possible flip combinations of the input tile:
     * - No flip (original orientation)
     * - Horizontal flip only
     * - Vertical flip only
     * - Both horizontal and vertical flip
     *
     * The lexicographically minimal tile among these four variants is selected as the canonical form and stored as the
     * base ShapeTile data. The flip flags that transform this canonical form back to the input tile are stored in
     * h_flip_ and v_flip_.
     *
     * The lexicographic comparison is performed using ShapeTile's custom operator<, which compares ONLY the shape
     * masks (geometry) and ignores pixel values. This means two tiles with identical shapes but different color
     * assignments will canonicalize to the same shape structure (though their pixel values will differ).
     *
     * @param tile The input ShapeTile to canonicalize
     */
    CanonicalShapeTile(const ShapeTile<PixelType> &tile) : ShapeTile<PixelType>{}
    {
        // Helper struct to store candidate tiles with their flip flags
        struct Candidate {
            ShapeTile<PixelType> flipped_tile;
            bool h_flip;
            bool v_flip;

            bool operator<(const Candidate &other) const
            {
                return flipped_tile < other.flipped_tile;
            }
        };

        std::array flips = {
            std::pair{false, false}, std::pair{false, true}, std::pair{true, false}, std::pair{true, true}};

        std::vector<Candidate> candidates;
        candidates.reserve(4);

        for (const auto &[h, v] : flips) {
            candidates.push_back({tile.flip(h, v), h, v});
        }

        auto min_candidate = *std::min_element(candidates.begin(), candidates.end());

        // Assign the canonical tile data
        *static_cast<ShapeTile<PixelType> *>(this) = min_candidate.flipped_tile;
        h_flip_ = min_candidate.h_flip;
        v_flip_ = min_candidate.v_flip;
    }

    /**
     * @brief Equality comparison operator that compares all fields.
     *
     * @details
     * Two CanonicalShapeTile instances are equal if and only if they have identical canonical tile data (both shape
     * masks AND pixel values) and identical flip flags. Uses ShapeTile's defaulted operator== which compares both
     * keys and values in the underlying std::map.
     *
     * @param other The CanonicalShapeTile to compare against
     * @return True if equal, false otherwise
     */
    bool operator==(const CanonicalShapeTile &other) const = default;

    /**
     * @brief Three-way comparison operator that compares all fields including pixel values.
     *
     * @details
     * This operator is consistent with operator== and compares:
     * 1. Shape masks AND pixel values (full ShapeTile equality check)
     * 2. If ShapeTiles are fully equal, then compares h_flip_
     * 3. If h_flip_ values are equal, then compares v_flip_
     *
     * Semantic consistency guarantee:
     * - If (a <=> b) == 0, then (a == b) will be true
     * - If (a == b) is true, then (a <=> b) == 0
     *
     * This ensures that CanonicalShapeTile instances can be safely used in ordered containers (std::set, std::map)
     * with expected behavior: two tiles are considered equivalent by the container only if they have identical
     * shape masks, identical pixel values, and identical flip flags.
     *
     * Note: When ShapeTiles differ, ordering is established using ShapeTile's custom operator< which compares
     * only shape masks (geometry). This means tiles with different colors but the same shape structure will be
     * ordered based on their shape geometry, but will NOT be considered equal unless their colors also match.
     *
     * @param other The CanonicalShapeTile to compare against
     * @return A std::strong_ordering indicating less than, equal to, or greater than relationship
     */
    auto operator<=>(const CanonicalShapeTile &other) const
    {
        // Compare ShapeTile part for full equality (shape masks AND pixel values)
        const auto &this_shape = static_cast<const ShapeTile<PixelType> &>(*this);
        const auto &other_shape = static_cast<const ShapeTile<PixelType> &>(other);

        if (this_shape == other_shape) {
            // ShapeTiles are fully equal (same shape masks AND same pixel values), compare flip flags
            if (auto cmp = h_flip_ <=> other.h_flip_; cmp != 0) {
                return cmp;
            }
            return v_flip_ <=> other.v_flip_;
        }

        // ShapeTiles differ - use shape-based ordering to establish less/greater relationship
        if (this_shape < other_shape) {
            return std::strong_ordering::less;
        }
        return std::strong_ordering::greater;
    }

    /**
     * @brief Returns the horizontal flip flag.
     *
     * @details
     * Indicates whether the canonical tile must be horizontally flipped to reproduce the original input tile. If true,
     * applying a horizontal flip to this CanonicalShapeTile's tile data will yield the original tile orientation.
     *
     * @return True if horizontal flip is needed to reconstruct the original tile, false otherwise
     */
    [[nodiscard]] bool h_flip() const
    {
        return h_flip_;
    }

    /**
     * @brief Returns the vertical flip flag.
     *
     * @details
     * Indicates whether the canonical tile must be vertically flipped to reproduce the original input tile. If true,
     * applying a vertical flip to this CanonicalShapeTile's tile data will yield the original tile orientation.
     *
     * @return True if vertical flip is needed to reconstruct the original tile, false otherwise
     */
    [[nodiscard]] bool v_flip() const
    {
        return v_flip_;
    }

  private:
    bool h_flip_;
    bool v_flip_;
};

} // namespace porytiles2
