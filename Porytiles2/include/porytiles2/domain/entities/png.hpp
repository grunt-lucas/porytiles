#pragma once

#include "porytiles2/domain/value_objects/rgba32.hpp"

namespace porytiles {

/**
 * @brief An image in PNG format.
 *
 * @details
 * Clients who need to operate on PNG images can use this interface to avoid dependencies on any particular PNG library.
 * This interface provides a way for clients to read a PNG's metadata and manipulate its contents.
 */
class Png {
  public:
    virtual ~Png() = default;

    /**
     * @brief Gets the width of this Png in pixels.
     *
     * @return The width of this Png in pixels.
     */
    [[nodiscard]] virtual std::size_t Width() const = 0;

    /**
     * @brief Gets the height of this Png in pixels.
     *
     * @return The height of this Png in pixels.
     */
    [[nodiscard]] virtual std::size_t Height() const = 0;

    /**
     * @brief Fetches the Rgba32 color value at a given one-dimensional pixel index.
     *
     * @details
     * The one-dimensional index assumes the Png as an array of pixels, where the length of the array is the Png's width
     * times height.
     *
     * @param i The one-dimensional pixel index.
     * @return The Rgba32 at the given pixel index.
     */
    [[nodiscard]] virtual Rgba32 At(std::size_t i) const = 0;

    /**
     * @brief Fetches the Rgba32 color value at a given row and column.
     *
     * @param row The pixel row.
     * @param col The pixel column.
     * @return The Rgba32 at the given row and column.
     */
    [[nodiscard]] virtual Rgba32 At(std::size_t row, std::size_t col) const = 0;
};

} // namespace porytiles
