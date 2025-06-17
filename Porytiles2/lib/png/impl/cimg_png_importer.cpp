#include "cimg_png.hpp"

#include <png/impl/cimg_png_importer.hpp>

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

#include <CImg.h>

namespace porytiles {

using cimg_library::CImg;
using cimg_library::CImgException;

std::expected<std::unique_ptr<Png>, std::string> CImgPngImporter::Read(const std::filesystem::path &path) const {
    CImg<std::uint8_t> image{};
    const auto path_c_str = path.c_str();
    try {
        image.assign(path_c_str);
        return std::make_unique<CImgPng>(image);
    } catch (const CImgException &e) {
        return std::unexpected{e.what()};
    }
}

} // namespace porytiles