#include "png/cimg_png.hpp"

namespace porytiles {

std::expected<void, std::string> CImgPng::Read(const std::filesystem::path &path) {
    const auto path_c_str = path.c_str();
    try {
        image_ = cimg_library::CImg<std::uint8_t>(path_c_str);
        return {};
    } catch (const cimg_library::CImgException &e) {
        return std::unexpected{e.what()};
    }
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
