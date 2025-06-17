#include <png/impl/png_impl.hpp>

#include <porytiles2/panic/panic.hpp>

namespace porytiles {

using cimg_library::CImg;
using cimg_library::CImgException;

PngImpl::PngImpl(const CImg<std::uint8_t> &image) {
    if (image.spectrum() != 3 && image.spectrum() != 4) {
        Panic("CImgPng only supports 3 or 4 channel images");
    }
    image_.assign(image);
}

std::size_t PngImpl::Width() const {
    return static_cast<std::size_t>(image_.width());
}

std::size_t PngImpl::Height() const {
    return static_cast<std::size_t>(image_.height());
}

Rgba32 PngImpl::At(std::size_t i) const {
    Panic("not implemented");
}

Rgba32 PngImpl::At(const std::size_t row, const std::size_t col) const {
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
