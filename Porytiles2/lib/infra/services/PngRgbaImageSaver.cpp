#include "porytiles2/infra/services/PngRgbaImageSaver.hpp"

#include <expected>
#include <filesystem>
#include <string>

#include "CImg.h"
#include "fmt/format.h"

#include "porytiles2/domain/model/valueobj/RgbaImage.hpp"

namespace porytiles {

using cimg_library::CImg;
using cimg_library::CImgException;

std::expected<void, std::string>
PngRgbaImageSaver::SaveToFile(const RgbaImage &image, const std::filesystem::path &path) const {
  const auto width = static_cast<int>(image.width());
  const auto height = static_cast<int>(image.height());
  constexpr auto spectrum = 4; // RGBA

  // Cannot use braced initializer here, it confuses the compiler
  CImg<std::uint8_t> cimg_png(width, height, 1, spectrum);

  for (std::size_t row = 0; row < image.height(); ++row) {
    for (std::size_t col = 0; col < image.width(); ++col) {
      const auto pixel = image.At(row, col);

      cimg_png(col, row, 0, 0) = pixel.red();
      cimg_png(col, row, 0, 1) = pixel.green();
      cimg_png(col, row, 0, 2) = pixel.blue();
      cimg_png(col, row, 0, 3) = pixel.alpha();
    }
  }

  const auto path_c_str = path.c_str();
  try {
    std::ignore = cimg_png.save_png(path_c_str);
  } catch (const CImgException &e) {
    return std::unexpected{fmt::format("Failed to save PNG to {}: {}", path_c_str, e.what())};
  }

  return {};
}

} // namespace porytiles