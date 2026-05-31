#pragma once

#include <algorithm>
#include <array>
#include <vector>

#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/supports_transparency.hpp"

namespace porytiles {

/**
 * @brief A PixelTile representation that stores the canonical (lexicographically minimal) orientation among all flipped
 * variants.
 *
 * @details
 * CanonicalPixelTile finds and stores the canonical orientation of a tile by evaluating all four possible flip
 * combinations (no flip, horizontal only, vertical only, and both horizontal and vertical). The canonical orientation
 * is defined as the lexicographically minimal tile data among these four variants.
 *
 * This representation enables:
 * - Tile deduplication: tiles that are identical under flipping can be recognized as equivalent
 * - Canonical comparison: tiles can be compared in a normalized form regardless of their original orientation
 * - Orientation tracking: the flip flags record how to transform the canonical form back to the original orientation
 *
 * The stored flip flags (h_flip_ and v_flip_) represent the transformation needed to convert the canonical tile back
 * to the original input tile. For example, if the input tile's horizontal flip is its canonical form, h_flip() will
 * return true, indicating that horizontally flipping the canonical tile reproduces the original.
 *
 * This approach is particularly useful for:
 * - Tileset generation where tiles should be deduplicated regardless of orientation
 * - Hardware targeting that supports tile flipping (e.g., GBA hardware attributes)
 * - Optimizing tileset size by identifying tiles that are rotations/flips of each other
 *
 * @tparam PixelType The pixel type of this tile; must satisfy SupportsTransparency concept
 */
template <SupportsTransparency PixelType>
class CanonicalPixelTile : public PixelTile<PixelType> {
  public:
    /**
     * @brief Constructs a CanonicalPixelTile by finding the canonical orientation of the input tile.
     *
     * @details
     * This constructor evaluates all four possible flip combinations of the input tile:
     * - No flip (original orientation)
     * - Horizontal flip only
     * - Vertical flip only
     * - Both horizontal and vertical flip
     *
     * The lexicographically minimal tile among these four variants is selected as the canonical form and stored as the
     * base PixelTile data. The flip flags that transform this canonical form back to the input tile are stored in
     * h_flip_ and v_flip_.
     *
     * The lexicographic comparison is performed using the default PixelTile comparison operator (operator<=>), which
     * compares pixel data element-by-element.
     *
     * @param tile The input PixelTile to canonicalize
     */
    explicit CanonicalPixelTile(const PixelTile<PixelType> &tile) : PixelTile<PixelType>{}
    {
        // Helper struct to store candidate tiles with their flip flags
        struct Candidate {
            PixelTile<PixelType> flipped_tile;
            bool h_flip;
            bool v_flip;

            auto operator<=>(const Candidate &other) const
            {
                return flipped_tile <=> other.flipped_tile;
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
        *static_cast<PixelTile<PixelType> *>(this) = min_candidate.flipped_tile;
        h_flip_ = min_candidate.h_flip;
        v_flip_ = min_candidate.v_flip;
    }

    /**
     * @brief Canonicalizes the input tile's orientation under cross-ET strict weak ordering.
     *
     * @details
     * Picks the lex-min orientation among the four flip combinations, using
     * @c PixelTile<PixelType>::cross_et_compare with @p extrinsic_transparency supplied as both sides of the
     * comparison. This produces an orientation that is stable across ET values: two tiles whose opaque-pixel
     * patterns coincide but whose transparent pixels differ by ET will canonicalize to equivalent orientations
     * (equivalent under @c cross_et_compare), which is the precondition for matching them in ET-aware lookup maps.
     *
     * Only enabled for pixel types that support parameterized @c is_transparent (e.g., @c Rgba32). This constructor
     * exists to support cross-tileset animation matching, where tiles compiled under different extrinsic
     * transparency values must canonicalize to equivalent orientations so the matcher's cross-ET comparator finds
     * them equal.
     *
     * @param tile The input PixelTile to canonicalize.
     * @param extrinsic_transparency The extrinsic transparency value to use for cross-ET comparison during
     * canonicalization (applied to both sides, since we are comparing the tile's own flipped variants).
     */
    CanonicalPixelTile(const PixelTile<PixelType> &tile, const PixelType &extrinsic_transparency)
        requires requires(const PixelType &p) { p.is_transparent(p); }
        : PixelTile<PixelType>{}
    {
        struct Candidate {
            PixelTile<PixelType> flipped_tile;
            bool h_flip;
            bool v_flip;
        };

        std::array flips = {
            std::pair{false, false}, std::pair{false, true}, std::pair{true, false}, std::pair{true, true}};

        std::vector<Candidate> candidates;
        candidates.reserve(4);
        for (const auto &[h, v] : flips) {
            candidates.push_back({tile.flip(h, v), h, v});
        }

        auto min_it = std::min_element(
            candidates.begin(), candidates.end(), [&extrinsic_transparency](const Candidate &a, const Candidate &b) {
                return PixelTile<PixelType>::cross_et_compare(
                           a.flipped_tile, extrinsic_transparency, b.flipped_tile, extrinsic_transparency) < 0;
            });

        *static_cast<PixelTile<PixelType> *>(this) = min_it->flipped_tile;
        h_flip_ = min_it->h_flip;
        v_flip_ = min_it->v_flip;
    }

    /**
     * @brief Three-way comparison operator that compares all fields in lexicographic order.
     *
     * @details
     * Provides complete ordering by comparing first the base PixelTile data, then h_flip_, then v_flip_. This enables
     * CanonicalPixelTile instances to be used in ordered containers like std::set or std::map.
     *
     * Two CanonicalPixelTile instances are equal if and only if they have identical canonical tile data and identical
     * flip flags.
     *
     * @param other The CanonicalPixelTile to compare against
     * @return A comparison category indicating less than, equal to, or greater than relationship
     */
    auto operator<=>(const CanonicalPixelTile &other) const = default;

    /**
     * @brief Returns the horizontal flip flag.
     *
     * @details
     * Indicates whether the canonical tile must be horizontally flipped to reproduce the original input tile. If true,
     * applying a horizontal flip to this CanonicalPixelTile's tile data will yield the original tile orientation.
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
     * applying a vertical flip to this CanonicalPixelTile's tile data will yield the original tile orientation.
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

} // namespace porytiles
