#pragma once

#include <vector>

#include "porytiles2/domain/model/valueobj/rgba32.hpp"

namespace porytiles2 {

/**
 * @brief An image with Rgba32 pixel values.
 *
 * @details
 * Clients who need to operate on RGB images can use this class to read image data and manipulate image contents. This
 * value type makes no assumptions about the underlying image storage format.
 */
class RgbaImage {
  public:
    RgbaImage(std::size_t width, std::size_t height);

    /**
     * @brief Fetches the Rgba32 color value at a given one-dimensional pixel
     * index.
     *
     * @details
     * The one-dimensional index assumes the image as an array of pixels, where
     * the length of the array is the image's width times height.
     *
     * @param i The one-dimensional pixel index.
     * @return The Rgba32 at the given pixel index.
     */
    [[nodiscard]] Rgba32 at(std::size_t i) const;

    /**
     * @brief Fetches the Rgba32 color value at a given row and column.
     *
     * @param row The pixel row.
     * @param col The pixel column.
     * @return The Rgba32 at the given row and column.
     */
    [[nodiscard]] Rgba32 at(std::size_t row, std::size_t col) const;

    /**
     * @brief Sets the Rgba32 color value at a given one-dimensional pixel index.
     *
     * @param i The one-dimensional pixel index.
     * @param pixel The Rgba32 to set at the given pixel index.
     */
    void set(std::size_t i, const Rgba32 &pixel);

    /**
     * @brief Sets the Rgba32 color value at a given row and column.
     *
     * @param row The pixel row.
     * @param col The pixel column.
     * @param pixel The Rgba32 to set at the given pixel row and column.
     */
    void set(std::size_t row, std::size_t col, const Rgba32 &pixel);

    /**
     * @brief Gets the width of this image in pixels.
     *
     * @return The width of this image in pixels.
     */
    [[nodiscard]] std::size_t width() const {
        return width_;
    }

    /**
     * @brief Gets the height of this image in pixels.
     *
     * @return The height of this image in pixels.
     */
    [[nodiscard]] std::size_t height() const {
        return height_;
    }

  private:
    std::vector<Rgba32> pixels_;
    std::size_t width_;
    std::size_t height_;
};

} // namespace porytiles2
