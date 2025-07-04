#include "porytiles2/infra/repos/PngRgbaImageRepo.hpp"

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

#include "CImg.h"
#include "fmt/format.h"

#include "porytiles2/domain/model/valueobj/RgbaImage.hpp"

namespace porytiles {

using cimg_library::CImg;
using cimg_library::CImgException;

std::expected<std::unique_ptr<RgbaImage>, std::string>
PngRgbaImageRepo::Read(const std::filesystem::path &path) const {
  CImg<std::uint8_t> cimg_png{};
  const auto path_c_str = path.c_str();
  try {
    cimg_png.assign(path_c_str);
  } catch (const CImgException &e) {
    return std::unexpected{e.what()};
  }

  if (cimg_png.spectrum() != 3 && cimg_png.spectrum() != 4) {
    return std::unexpected{
        fmt::format("{}: CImgPng repo only supports 3 or 4 channel images", path_c_str)};
  }

  const auto width = static_cast<std::size_t>(cimg_png.width());
  const auto height = static_cast<std::size_t>(cimg_png.height());

  RgbaImage image{width, height};

  for (std::size_t row = 0; row < height; ++row) {
    for (std::size_t col = 0; col < width; ++col) {
      const auto red = cimg_png(col, row, 0, 0);
      const auto green = cimg_png(col, row, 0, 1);
      const auto blue = cimg_png(col, row, 0, 2);

      // PNGs with no alpha channel are considered opaque
      const auto alpha =
          (cimg_png.spectrum() == 3) ? Rgba32::kAlphaOpaque : cimg_png(col, row, 0, 3);

      image.Set(row, col, Rgba32{red, green, blue, alpha});
    }
  }

  return std::make_unique<RgbaImage>(std::move(image));
}

} // namespace porytiles
