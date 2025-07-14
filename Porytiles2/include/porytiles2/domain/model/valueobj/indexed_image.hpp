#pragma once

#include <vector>

namespace porytiles2 {

/**
 * @brief An image with pixel values that are indexed into an internal palette.
 *
 * @details
 * Clients who need to operate on indexed images can use this class to read image data and manipulate image contents.
 * This value type makes no assumptions about the underlying image storage format.
 */
class IndexedImage {
  public:
    IndexedImage(std::size_t width, std::size_t height);

    /**
     * @brief Fetches the index value at a given one-dimensional pixel index.
     *
     * @details
     * The one-dimensional index assumes the image as an array of pixels, where the length of the array is the image's
     * width times height.
     *
     * @param i The one-dimensional pixel index.
     * @return The int at the given pixel index.
     */
    [[nodiscard]] int at(std::size_t i) const;

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
    std::vector<int> pixels_;
    std::size_t width_;
    std::size_t height_;
};

} // namespace porytiles2
