#include "porytiles2/infra/services/png_rgba_image_saver.hpp"

#include <expected>
#include <filesystem>
#include <format>
#include <string>

#include "CImg.h"

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

using cimg_library::CImg;
using cimg_library::CImgException;

ChainableResult<void>
PngRgbaImageSaver::save_to_file(const Image<Rgba32> &image, const std::filesystem::path &path) const
{
    const auto width = static_cast<int>(image.width());
    const auto height = static_cast<int>(image.height());
    constexpr auto spectrum = 4; // RGBA

    // Cannot use braced initializer here, it confuses the compiler
    CImg<std::uint8_t> cimg_png(width, height, 1, spectrum);

    for (std::size_t row = 0; row < image.height(); ++row) {
        for (std::size_t col = 0; col < image.width(); ++col) {
            const auto pixel = image.at(row, col);

            cimg_png(col, row, 0, 0) = pixel.red();
            cimg_png(col, row, 0, 1) = pixel.green();
            cimg_png(col, row, 0, 2) = pixel.blue();
            cimg_png(col, row, 0, 3) = pixel.alpha();
        }
    }

    const auto path_c_str = path.c_str();
    try {
        std::ignore = cimg_png.save_png(path_c_str);
    }
    catch (const std::exception &e) {
        return FormattableError{std::format("{}: save failed: {}", path.filename().c_str(), e.what())};
    }

    return {};
}

} // namespace porytiles2