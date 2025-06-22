#include <porytiles2/infra/png/cimg_png.hpp>

#include <CImg.h>

#include <porytiles2/templates/panic.hpp>

namespace porytiles {

using cimg_library::CImg;
using cimg_library::CImgException;

CImgPng::CImgPng(const CImg<std::uint8_t> &image) {
    if (image.spectrum() != 3 && image.spectrum() != 4) {
        Panic("CImgPng only supports 3 or 4 channel images");
    }
    image_.assign(image);
}

std::size_t CImgPng::Width() const {
    return static_cast<std::size_t>(image_.width());
}

std::size_t CImgPng::Height() const {
    return static_cast<std::size_t>(image_.height());
}

Rgba32 CImgPng::At(std::size_t i) const {
    Panic("not implemented");
}

Rgba32 CImgPng::At(const std::size_t row, const std::size_t col) const {
    if (col >= Width()) {
        Panic(fmt::format("col {} out of bounds for PNG width {}", col, Width()));
    }
    if (row >= Height()) {
        Panic(fmt::format("row {} out of bounds for PNG height {}", row, Height()));
    }
    const auto red = image_(col, row, 0, 0);
    const auto green = image_(col, row, 0, 1);
    const auto blue = image_(col, row, 0, 2);

    // PNGs with no alpha channel are considered opaque
    if (image_.spectrum() == 3) {
        return Rgba32{red, green, blue, Rgba32::kAlphaOpaque};
    }

    const auto alpha = image_(col, row, 0, 3);
    return Rgba32{red, green, blue, alpha};
}

} // namespace porytiles
