#pragma once

#include "porytiles2/domain/valueobj/Rgba32.hpp"

namespace porytiles {

/**
 * @brief An image with Rgba32 pixel values.
 *
 * @details
 * Clients who need to operate on RGB images can use this interface to avoid
 * dependencies on any particular image library. This interface provides a way
 * for clients to read image metadata and manipulate its contents.
 */
class RgbaImage {
public:
  virtual ~RgbaImage() = default;

  /**
   * @brief Gets the width of this image in pixels.
   *
   * @return The width of this image in pixels.
   */
  [[nodiscard]] virtual std::size_t Width() const = 0;

  /**
   * @brief Gets the height of this image in pixels.
   *
   * @return The height of this image in pixels.
   */
  [[nodiscard]] virtual std::size_t Height() const = 0;

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
