#pragma once

#include <set>
#include <vector>

#include "porytiles2/domain/model/rgba32.hpp"

namespace porytiles2 {

/**
 * @brief A palette representation that preserves insertion order while supporting unordered equality checks.
 *
 * @details
 * UnorderedPal is designed for use with IsoColorTile to detect isomorphism under color transformations. It maintains
 * colors in insertion order (via a vector) while also tracking uniqueness (via a set). This allows the palette to be
 * used for computing color mappings (where order matters for alignment) while also supporting equality checks that
 * ignore order (to distinguish between iso-under-flip and iso-under-color cases).
 *
 * The palette also stores an extrinsic transparency color that represents which color should be treated as transparent
 * for this palette's context.
 */
class UnorderedPal {
  public:
    /**
     * @brief Constructs an UnorderedPal with a specified extrinsic transparency.
     *
     * @details
     * The extrinsic transparency color defines which color should be treated as transparent when working with tiles
     * using this palette.
     *
     * @param extrinsic The color to treat as transparent
     */
    explicit UnorderedPal(const Rgba32 &extrinsic) : extrinsic_transparency_{extrinsic} {}

    /**
     * @brief Inserts a color into the palette.
     *
     * @details
     * Colors are appended to the internal vector in insertion order. Duplicate colors are tracked via an internal set
     * but may appear multiple times in the ordered color vector.
     *
     * @param color The color to insert
     */
    void insert(const Rgba32 &color);

    /**
     * @brief Returns the number of colors in the palette.
     *
     * @return The count of colors stored in this palette
     */
    [[nodiscard]] std::size_t size() const;

    // TODO: this function needs a better name
    /**
     * @brief Checks if another UnorderedPal's colors are identical to this one, disregarding order.
     *
     * @details
     * Two UnorderedPals have identical contents if their color vectors contain the same colors, regardless of order.
     * The extrinsic transparency is not relevant for the identical content check.
     *
     * @param other the UnorderedPal to check against
     * @return true if both pals have identical content
     */
    [[nodiscard]] bool has_identical_content(const UnorderedPal &other) const;

    /**
     * @brief Returns the extrinsic transparency color for this palette.
     *
     * @details
     * The extrinsic transparency color defines which color should be treated as transparent in the context of this
     * palette.
     *
     * @return A const reference to the extrinsic transparency color
     */
    [[nodiscard]] const Rgba32 &extrinsic_transparency() const
    {
        return extrinsic_transparency_;
    }

    /**
     * @brief Returns the ordered color vector for this palette.
     *
     * @details
     * Colors are stored in insertion order. This ordering is critical for computing color mappings when determining
     * isomorphism between IsoColorTiles - the index alignment between two palettes defines the color transformation
     * function.
     *
     * @return A const reference to the vector of colors in insertion order
     */
    [[nodiscard]] const std::vector<Rgba32> &colors() const
    {
        return colors_;
    }

  private:
    Rgba32 extrinsic_transparency_;
    std::vector<Rgba32> colors_;
    std::set<Rgba32> unique_;
};

} // namespace porytiles2
