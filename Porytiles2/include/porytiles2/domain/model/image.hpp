#pragma once

#include <vector>

#include "fmt/format.h"

#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

/**
 * @brief A template for two-dimensional images with arbitrarily-typed pixel values.
 *
 * @details
 * Clients who need to operate on images can use this class to read image data and manipulate image contents. This value
 * type makes no assumptions about the underlying image storage format.
 *
 * @tparam P The pixel type for this Image
 */
template <typename P>
class Image {
  public:
    Image(std::size_t width, std::size_t height) {
        width_ = width;
        height_ = height;
        pixels_.resize(width * height);
    }

    /**
     * @brief Fetches the pixel value at a given one-dimensional pixel index.
     *
     * @details
     * The one-dimensional index assumes the image as an array of pixels, where the length of the array is the image's
     * width times height.
     *
     * @param i The one-dimensional pixel index.
     * @return The pixel value at the given pixel index.
     */
    [[nodiscard]] P at(std::size_t i) const {
        if (const auto size = width_ * height_; i >= size) {
            panic(fmt::format("index {} out of bounds for image size {}", i, size));
        }
        return pixels_[i];
    }

    /**
     * @brief Fetches the pixel value at a given row and column.
     *
     * @param row The pixel row.
     * @param col The pixel column.
     * @return The pixel value at the given row and column.
     */
    [[nodiscard]] P at(std::size_t row, std::size_t col) const {
        if (col >= width_) {
            panic(fmt::format("col {} out of bounds for image width {}", col, width_));
        }
        if (row >= height_) {
            panic(fmt::format("row {} out of bounds for image height {}", row, height_));
        }
        return pixels_[row * width_ + col];
    }

    /**
     * @brief Sets the pixel value at a given one-dimensional pixel index.
     *
     * @param i The one-dimensional pixel index.
     * @param pixel The pixel value to set at the given pixel index.
     */
    void set(std::size_t i, P pixel) {
        if (const auto size = width_ * height_; i >= size) {
            panic(fmt::format("index {} out of bounds for image size {}", i, size));
        }
        pixels_[i] = pixel;
    }

    /**
     * @brief Sets the pixel value at a given row and column.
     *
     * @param row The pixel row.
     * @param col The pixel column.
     * @param pixel The pixel value to set at the given pixel row and column.
     */
    void set(std::size_t row, std::size_t col, P pixel) {
        if (col >= width_) {
            panic(fmt::format("col {} out of bounds for image width {}", col, width_));
        }
        if (row >= height_) {
            panic(fmt::format("row {} out of bounds for image height {}", row, height_));
        }
        pixels_[row * width_ + col] = pixel;
    }

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
    std::vector<P> pixels_;
    std::size_t width_;
    std::size_t height_;
};

} // namespace porytiles2
