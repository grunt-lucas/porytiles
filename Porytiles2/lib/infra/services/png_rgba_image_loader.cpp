#include "porytiles2/infra/services/png_rgba_image_loader.hpp"

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

#include "CImg.h"
#include "fmt/format.h"

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

using cimg_library::CImg;
using cimg_library::CImgException;

TraceableResult<std::unique_ptr<Image<Rgba32>>, ImageLoadError>
PngRgbaImageLoader::load_from_file(const std::filesystem::path &path) const
{
    using enum ImageLoadError::Type;

    if (!exists(path)) {
        return ImageLoadError::file_not_found(path.string());
    }

    CImg<std::uint8_t> cimg_png{};
    const auto path_c_str = path.c_str();
    try {
        cimg_png.assign(path_c_str);
    }
    catch (const CImgException &e) {
        return ImageLoadError::other_load_error(path.string(), e.what());
    }

    if (cimg_png.spectrum() != 3 && cimg_png.spectrum() != 4) {
        return ImageLoadError::unsupported_channel_count(path.string(), cimg_png.spectrum());
    }

    const auto width = static_cast<std::size_t>(cimg_png.width());
    const auto height = static_cast<std::size_t>(cimg_png.height());

    Image<Rgba32> image{width, height};

    for (std::size_t row = 0; row < height; ++row) {
        for (std::size_t col = 0; col < width; ++col) {
            const auto red = cimg_png(col, row, 0, 0);
            const auto green = cimg_png(col, row, 0, 1);
            const auto blue = cimg_png(col, row, 0, 2);

            // PNGs with no alpha channel are considered opaque
            const auto alpha = (cimg_png.spectrum() == 3) ? Rgba32::alpha_opaque : cimg_png(col, row, 0, 3);

            image.set(row, col, Rgba32{red, green, blue, alpha});
        }
    }

    return std::make_unique<Image<Rgba32>>(std::move(image));
}

} // namespace porytiles2