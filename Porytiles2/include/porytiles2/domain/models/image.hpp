#pragma once

#include <format>
#include <optional>
#include <vector>

#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief A template for two-dimensional images with arbitrarily typed pixel values.
 *
 * @details
 * Clients who need to operate on images can use this class to read image data and manipulate image contents. This value
 * type makes no assumptions about the underlying image storage format. Image provides an optional palette field for
 * images which want to store an IndexPixel pixel type into a fixed palette of colors.
 *
 * @tparam PixelType The pixel type for this Image
 */
template <typename PixelType>
class Image {
  public:
    Image() : Image(0, 0) {}

    Image(std::size_t width, std::size_t height) : pixels_(width * height), width_{width}, height_{height} {}

    Image(std::size_t width, std::size_t height, std::vector<Rgba32> palette)
        : pixels_(width * height), palette_(std::move(palette)), width_{width}, height_{height}
    {
    }

    /**
     * @brief Constructs an image with all pixels set to a specified fill value.
     *
     * @param width The width of the image in pixels.
     * @param height The height of the image in pixels.
     * @param fill_color The pixel value to fill the entire image with.
     */
    Image(std::size_t width, std::size_t height, PixelType fill_color)
        : pixels_(width * height, fill_color), width_{width}, height_{height}
    {
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
    [[nodiscard]] PixelType at(std::size_t i) const
    {
        if (const auto s = size(); i >= s) {
            panic(std::format("index {} out of bounds for image size {}", i, s));
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
    [[nodiscard]] PixelType at(std::size_t row, std::size_t col) const
    {
        if (col >= width_) {
            panic(std::format("col {} out of bounds for image width {}", col, width_));
        }
        if (row >= height_) {
            panic(std::format("row {} out of bounds for image height {}", row, height_));
        }
        return pixels_[row * width_ + col];
    }

    /**
     * @brief Sets the pixel value at a given one-dimensional pixel index.
     *
     * @param i The one-dimensional pixel index.
     * @param pixel The pixel value to set at the given pixel index.
     */
    void set(std::size_t i, PixelType pixel)
    {
        if (const auto s = size(); i >= s) {
            panic(std::format("index {} out of bounds for image size {}", i, s));
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
    void set(std::size_t row, std::size_t col, PixelType pixel)
    {
        if (col >= width_) {
            panic(std::format("col {} out of bounds for image width {}", col, width_));
        }
        if (row >= height_) {
            panic(std::format("row {} out of bounds for image height {}", row, height_));
        }
        pixels_[row * width_ + col] = pixel;
    }

    /**
     * @brief Gets the width of this image in pixels.
     *
     * @return The width of this image in pixels.
     */
    [[nodiscard]] std::size_t width() const
    {
        return width_;
    }

    /**
     * @brief Gets the height of this image in pixels.
     *
     * @return The height of this image in pixels.
     */
    [[nodiscard]] std::size_t height() const
    {
        return height_;
    }

    /**
     * @brief Gets the size of this image in pixels.
     *
     * @return The size of this image in pixels.
     */
    [[nodiscard]] std::size_t size() const
    {
        return pixels_.size();
    }

    /**
     * @brief Gets the number of 8x8 tile regions in this image.
     *
     * @details
     * GBA tiles are 8x8 pixels. This method calculates how many such tile regions fit in the image.
     *
     * @pre Image width must be divisible by 8.
     * @pre Image height must be divisible by 8.
     * @return The number of 8x8 tile regions in this image.
     */
    [[nodiscard]] std::size_t size_in_tiles() const
    {
        if (width_ % 8 != 0) {
            panic(std::format("image width {} is not divisible by 8", width_));
        }
        if (height_ % 8 != 0) {
            panic(std::format("image height {} is not divisible by 8", height_));
        }
        return (width_ / 8) * (height_ / 8);
    }

    [[nodiscard]] const std::optional<std::vector<Rgba32>> &palette() const
    {
        return palette_;
    }

    void palette(std::vector<Rgba32> pal)
    {
        palette_ = std::move(pal);
    }

  private:
    std::vector<PixelType> pixels_;
    // TODO: is there a better way to handle this?
    std::optional<std::vector<Rgba32>> palette_;
    std::size_t width_;
    std::size_t height_;
};

} // namespace porytiles2
