#include "porytiles2/infra/repos/CImgPngRgbaImageRepo.hpp"

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
CImgPngRgbaImageRepo::Read(const std::filesystem::path &path) const {
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

  // TODO : finish the implementation, iterate over the CImg image and fill in the RgbaImage using
  // the Set method

  return std::make_unique<RgbaImage>(std::move(image));
}

// std::size_t RgbaImagePng::Width() const { return static_cast<std::size_t>(image_.width()); }
//
// std::size_t RgbaImagePng::Height() const { return static_cast<std::size_t>(image_.height()); }
//
// Rgba32 RgbaImagePng::At(std::size_t i) const { Panic("not implemented"); }
//
// Rgba32 RgbaImagePng::At(const std::size_t row, const std::size_t col) const {
//   if (col >= Width()) {
//     Panic(fmt::format("col {} out of bounds for PNG width {}", col, Width()));
//   }
//   if (row >= Height()) {
//     Panic(fmt::format("row {} out of bounds for PNG height {}", row, Height()));
//   }
//   const auto red = image_(col, row, 0, 0);
//   const auto green = image_(col, row, 0, 1);
//   const auto blue = image_(col, row, 0, 2);
//
//   // PNGs with no alpha channel are considered opaque
//   if (image_.spectrum() == 3) {
//     return Rgba32{red, green, blue, Rgba32::kAlphaOpaque};
//   }
//
//   const auto alpha = image_(col, row, 0, 3);
//   return Rgba32{red, green, blue, alpha};
// }

} // namespace porytiles