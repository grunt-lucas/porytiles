#pragma once

#include <CImg.h>

#include "porytiles2/domain/model/entities/RgbaImage.hpp"

namespace porytiles {

/**
 * @brief Implementation of the RgbaImage interface.
 */
class RgbaImageImpl final : public RgbaImage {
public:
  explicit RgbaImageImpl(const cimg_library::CImg<std::uint8_t> &image);

  [[nodiscard]] std::size_t Width() const override;

  [[nodiscard]] std::size_t Height() const override;

  [[nodiscard]] Rgba32 At(std::size_t i) const override;

  [[nodiscard]] Rgba32 At(std::size_t row, std::size_t col) const override;

private:
  cimg_library::CImg<std::uint8_t> image_;
};

} // namespace porytiles
