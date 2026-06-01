#include "porytiles/infra/services/png_rgba_image_saver.hpp"

#include <filesystem>
#include <format>

#include "png++/png.hpp"

#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

ChainableResult<void>
PngRgbaImageSaver::save_to_file(const Image<Rgba32> &image, const std::filesystem::path &path) const
{
    const auto width = static_cast<png::uint_32>(image.width());
    const auto height = static_cast<png::uint_32>(image.height());

    png::image<png::rgba_pixel> png{width, height};

    for (std::size_t row = 0; row < image.height(); ++row) {
        for (std::size_t col = 0; col < image.width(); ++col) {
            const auto pixel = image.at(row, col);
            png[row][col] = png::rgba_pixel{pixel.red(), pixel.green(), pixel.blue(), pixel.alpha()};
        }
    }

    try {
        png.write(path.string());
    }
    catch (const std::exception &e) {
        return FormattableError{std::format("{}: save failed: {}", path.filename().c_str(), e.what())};
    }

    return {};
}

} // namespace porytiles
