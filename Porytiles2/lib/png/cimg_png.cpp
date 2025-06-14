#include "png/cimg_png.hpp"

namespace porytiles {

using cimg_library::CImg;
using cimg_library::CImgException;

std::expected<void, std::string> CImgPng::Read(const std::filesystem::path &path) {
    const auto path_c_str = path.c_str();
    try {
        image_.assign(path_c_str);
        return {};
    } catch (const CImgException &e) {
        return std::unexpected{e.what()};
    }
}

void CImgPng::Reset(const std::size_t width, const std::size_t height) {
    image_.assign(width, height, 1, 4, 0);
}

/// @todo implement
std::expected<void, std::string> CImgPng::Write(const std::filesystem::path &path) {
    return {};
}

std::size_t CImgPng::Width() const {
    return static_cast<std::size_t>(image_.width());
}

std::size_t CImgPng::Height() const {
    return static_cast<std::size_t>(image_.height());
}

} // namespace porytiles
