#include <porytiles2/infra/persistence/cimg_png_repo.hpp>

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

#include <CImg.h>

#include <porytiles2/domain/entities/png.hpp>
#include <porytiles2/infra/png/cimg_png.hpp>

namespace porytiles {

using cimg_library::CImg;
using cimg_library::CImgException;

std::expected<std::unique_ptr<Png>, std::string> CImgPngRepo::Read(const std::filesystem::path &path) const {
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