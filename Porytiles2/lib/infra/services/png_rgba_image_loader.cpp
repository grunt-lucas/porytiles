#include "porytiles2/infra/services/png_rgba_image_loader.hpp"

#include <filesystem>
#include <memory>

#include "png++/png.hpp"

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/rgba32.hpp"

namespace porytiles2 {

ChainableResult<std::unique_ptr<Image<Rgba32>>, ImageLoadError>
PngRgbaImageLoader::load_from_file(const std::filesystem::path &path) const
{
    if (!exists(path)) {
        return ImageLoadError::file_not_found(path.string());
    }

    png::image<png::rgba_pixel> png{};
    try {
        png.read(path.string());
    }
    catch (const std::exception &e) {
        return ImageLoadError::other_load_error(path.string(), e.what());
    }

    const auto width = static_cast<std::size_t>(png.get_width());
    const auto height = static_cast<std::size_t>(png.get_height());

    Image<Rgba32> image{width, height};

    for (std::size_t row = 0; row < height; ++row) {
        for (std::size_t col = 0; col < width; ++col) {
            const auto &pixel = png[row][col];
            image.set(row, col, Rgba32{pixel.red, pixel.green, pixel.blue, pixel.alpha});
        }
    }

    return std::make_unique<Image<Rgba32>>(std::move(image));
}

} // namespace porytiles2
