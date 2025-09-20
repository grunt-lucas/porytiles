#pragma once

#include <set>

#include "porytiles2/domain/model/rgba32.hpp"

namespace porytiles2 {

/**
 * @brief A palette that stores color types in ascending order.
 *
 * @tparam ColorType The type of color objects to store in the palette
 */
template <typename ColorType>
class NormalizedPal {
  public:
    NormalizedPal() = default;

    /**
     * @brief Inserts a color into the palette.
     *
     * @details
     * Colors are automatically stored in sorted order using std::set.
     *
     * @param color The color to insert into the palette
     */
    void insert(const ColorType &color)
    {
        colors_.insert(color);
    }

    /**
     * @brief Returns the number of colors in the palette.
     *
     * @return The number of unique colors stored in the palette
     */
    [[nodiscard]] std::size_t size() const
    {
        return colors_.size();
    }

    [[nodiscard]] const ColorType &extrinsic_transparency() const
    {
        return extrinsic_transparency_;
    }

    void extrinsic_transparency(const ColorType &color)
    {
        extrinsic_transparency_ = color;
    }

    /**
     * @brief Returns a const reference to the underlying color set.
     *
     * @details
     * The colors are stored in ascending order as determined by ColorType's comparison operator.
     *
     * @return A const reference to the set containing all colors
     */
    [[nodiscard]] const std::set<ColorType> &colors() const
    {
        return colors_;
    }

  private:
    ColorType extrinsic_transparency_;
    std::set<ColorType> colors_;
};

} // namespace porytiles2
