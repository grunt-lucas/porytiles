#include "porytiles2/infra/repos/RgbaImageRepoImpl.hpp"

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

#include "CImg.h"

#include "porytiles2/domain/model/entities/RgbaImage.hpp"
#include "porytiles2/infra/image/RgbaImageImpl.hpp"

namespace porytiles {

using cimg_library::CImg;
using cimg_library::CImgException;

std::expected<std::unique_ptr<RgbaImage>, std::string>
RgbaImageRepoImpl::Read(const std::filesystem::path &path) const {
  CImg<std::uint8_t> image{};
  const auto path_c_str = path.c_str();
  try {
    image.assign(path_c_str);
    return std::make_unique<RgbaImageImpl>(image);
  } catch (const CImgException &e) {
    return std::unexpected{e.what()};
  }
}

} // namespace porytiles