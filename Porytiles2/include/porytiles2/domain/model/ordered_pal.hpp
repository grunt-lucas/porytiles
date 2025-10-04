#pragma once

#include <set>
#include <vector>

#include "porytiles2/domain/model/rgba32.hpp"

namespace porytiles2 {

/**
 * @brief A palette representation that maintains colors in a canonical sorted order.
 *
 * @details
 * OrderedPal is designed for use with IsoFlipTile to detect isomorphism under flip transformations. It stores colors
 * in a std::set, which automatically maintains them in a canonical sorted order based on Rgba32's comparison operators.
 * This ensures that two palettes with the same colors will always compare as equal regardless of insertion order,
 * making it ideal for identifying tiles that are isomorphic under flip transformations (where the same colors appear
 * in different arrangements due to flipping).
 *
 * The palette also stores an extrinsic transparency color that represents which color should be treated as transparent
 * for this palette's context.
 */
class OrderedPal {
  public:
    /**
     * @brief Constructs an OrderedPal with a specified extrinsic transparency.
     *
     * @details
     * The extrinsic transparency color defines which color should be treated as transparent when working with tiles
     * using this palette.
     *
     * @param extrinsic The color to treat as transparent
     */
    explicit OrderedPal(const Rgba32 &extrinsic) : extrinsic_transparency_{extrinsic} {}

    /**
     * @brief Inserts a color into the palette.
     *
     * @details
     * Colors are inserted into the internal set, which automatically maintains them in sorted order. Duplicate
     * insertions have no effect since sets only store unique elements.
     *
     * @param color The color to insert
     */
    void insert(const Rgba32 &color);

    /**
     * @brief Returns the number of unique colors in the palette.
     *
     * @return The count of unique colors stored in this palette
     */
    [[nodiscard]] std::size_t size() const;

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
     * @brief Returns the set of colors in this palette.
     *
     * @details
     * Colors are stored in a std::set, maintaining them in a canonical sorted order. This makes palette comparisons
     * order-independent, which is essential for detecting isomorphism under flip transformations.
     *
     * @return A const reference to the set of colors
     */
    [[nodiscard]] const std::set<Rgba32> &colors() const
    {
        return colors_;
    }

  private:
    Rgba32 extrinsic_transparency_;
    std::set<Rgba32> colors_;
};

} // namespace porytiles2
