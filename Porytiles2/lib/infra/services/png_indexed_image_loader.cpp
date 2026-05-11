#include "porytiles2/infra/services/png_indexed_image_loader.hpp"

#include <format>
#include <vector>

#include "png++/png.hpp"

#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<std::unique_ptr<Image<IndexPixel>>>
PngIndexedImageLoader::load_from_file(const std::filesystem::path &path) const
{
    if (!exists(path)) {
        return FormattableError{std::format("file does not exist: {}", path.string())};
    }

    try {
        // Do this here so if the source is not a PNG, we can catch and give a better error
        png::image<png::index_pixel> test{path};
    }
    catch (std::exception &) {
        return FormattableError{"'{}': File not a valid indexed PNG.", FormatParam{path.string(), Style::bold}};
    }

    png::image<png::index_pixel> png{path};
    const auto tilesheet_width = png.get_width();
    const auto tilesheet_height = png.get_height();
    const auto tilesheet_size = tilesheet_width * tilesheet_height;

    // Extract palette from PNG and convert to Rgba32
    const auto &png_pal = png.get_palette();
    std::vector<Rgba32> rgba_pal;
    rgba_pal.reserve(png_pal.size());
    for (const auto &color : png_pal) {
        rgba_pal.emplace_back(color.red, color.green, color.blue, Rgba32::alpha_opaque);
    }

    auto image = std::make_unique<Image<IndexPixel>>(tilesheet_width, tilesheet_height, std::move(rgba_pal));
    for (std::size_t pixel_index = 0; pixel_index < tilesheet_size; pixel_index++) {
        const auto row = pixel_index / tilesheet_width;
        const auto col = pixel_index % tilesheet_width;
        image->set(pixel_index, IndexPixel{png[row][col]});
    }

    return image;
}

} // namespace porytiles2
