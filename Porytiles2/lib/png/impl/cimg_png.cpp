#include <png/impl/cimg_png.hpp>

namespace porytiles {

using cimg_library::CImg;
using cimg_library::CImgException;

CImgPng::CImgPng(const CImg<std::uint8_t> &image) {
    image_.assign(image);
}

std::size_t CImgPng::Width() const {
    return static_cast<std::size_t>(image_.width());
}

std::size_t CImgPng::Height() const {
    return static_cast<std::size_t>(image_.height());
}

} // namespace porytiles
