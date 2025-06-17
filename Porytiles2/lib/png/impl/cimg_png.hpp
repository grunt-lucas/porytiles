#pragma once

#include <CImg.h>

#include <porytiles2/png/png.hpp>

namespace porytiles {

/**
 * @brief Implementation of Png using the CImg image processing library.
 */
class CImgPng final : public Png {
  public:
    explicit CImgPng(const cimg_library::CImg<std::uint8_t> &image);

    [[nodiscard]] std::size_t Width() const override;

    [[nodiscard]] std::size_t Height() const override;

    [[nodiscard]] Rgba32 At(std::size_t i) const override;

    [[nodiscard]] Rgba32 At(std::size_t row, std::size_t col) const override;

  private:
    cimg_library::CImg<std::uint8_t> image_;
};

} // namespace porytiles
