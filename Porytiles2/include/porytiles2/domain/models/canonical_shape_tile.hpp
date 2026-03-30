#pragma once

#include <algorithm>
#include <array>
#include <compare>
#include <vector>

#include "porytiles2/domain/models/shape_tile.hpp"

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
 * - Uses ShapeTile::compare_shape_only() which compares ONLY shape masks (geometry), not pixel values (colors)
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
     * The lexicographic comparison uses a two-phase approach: first, ShapeTile::compare_shape_only compares ONLY
     * shape masks (geometry), ignoring pixel values. If the shape comparison produces a tie (which occurs with
     * shapes that are symmetric under one or more flips), the full ShapeTile comparison (including pixel values)
     * is used as a tiebreaker. This ensures that two tiles which are flips of each other with identical colors
     * will select the same canonical form and produce identical color maps. This is critical for correct color version
     * deduplication in shape group analysis.
     *
     * @param tile The input ShapeTile to canonicalize
     */
    explicit CanonicalShapeTile(const ShapeTile<PixelType> &tile) : ShapeTile<PixelType>{}
    {
        // Helper struct to store candidate tiles with their flip flags
        struct Candidate {
            ShapeTile<PixelType> flipped_tile;
            bool h_flip;
            bool v_flip;

            /*
             * Two-phase strict weak ordering: shape first, then full map as tiebreaker. The shape-only phase
             * ensures non-symmetric shapes always select the geometrically minimal flip. The full-map fallback
             * handles symmetric shapes (where all flip variants have identical key sets) by also considering
             * color values. This guarantees that two tiles which are flips of each other with the same colors
             * will converge on the same canonical form rather than each keeping their own color arrangement.
             */
            bool operator<(const Candidate &other) const
            {
                if (ShapeTile<PixelType>::compare_shape_only(flipped_tile, other.flipped_tile)) {
                    return true;
                }
                if (ShapeTile<PixelType>::compare_shape_only(other.flipped_tile, flipped_tile)) {
                    return false;
                }
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
     * @brief Three-way comparison operator that compares all fields.
     *
     * @details
     * Performs member-wise comparison in declaration order:
     * 1. Base ShapeTile (shape masks AND pixel values via ShapeTile::operator<=>)
     * 2. h_flip_ flag
     * 3. v_flip_ flag
     *
     * This operator is consistent with operator==, ensuring that:
     * - If (a <=> b) == 0, then (a == b) is true
     * - If (a == b) is true, then (a <=> b) == 0
     *
     * @param other The CanonicalShapeTile to compare against
     * @return A std::strong_ordering indicating less than, equal to, or greater than relationship
     */
    auto operator<=>(const CanonicalShapeTile &other) const = default;

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
