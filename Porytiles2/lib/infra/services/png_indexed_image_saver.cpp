#include "porytiles2/infra/services/png_indexed_image_saver.hpp"

#include <filesystem>

#include "fmt/format.h"
#include "png++/png.hpp"

#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/infra/config/tiles_pal_mode.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<void> PngIndexedImageSaver::save_to_file(
    const Image<IndexPixel> &image, const std::filesystem::path &path, TilesPalMode mode) const
{
    using enum TilesPalMode;

    /*
     * TODO: make this configurable. Current pal has pure white as index 0 and pure black as index 15, which matches
     * vanilla game tilesets. OG porytiles used a slightly different greyscale pal, with pure black as index 0 and
     * different step intervals between colors.
     */
    const auto greyscale_pal = standard_greyscale_pal();

    // Bail if given path exists already and isn't a file (i.e. it's a directory)
    if (exists(path) && !is_regular_file(path)) {
        return FormattableError{fmt::format("{}: exists but is not a file", path.filename().c_str())};
    }

    // Determine which palette to use
    std::vector<Rgba32> palette_to_use;
    if (mode == true_color && image.palette().has_value()) {
        palette_to_use = image.palette().value();
    }
    else {
        // Use greyscale palette for greyscale mode OR when true_color mode but no image palette exists
        palette_to_use = greyscale_pal;
    }

    // Set up PNG palette
    png::palette png_pal{0};
    for (const auto &color : palette_to_use) {
        png_pal.emplace_back(color.red(), color.green(), color.blue());
    }

    // Generic lambda to write indexed PNG with any pixel type
    auto write_image = [&]<typename PixelType>(png::image<PixelType> &img) {
        img.set_palette(png_pal);
        for (unsigned int pixel_index = 0; pixel_index < image.size(); pixel_index++) {
            const auto row = pixel_index / image.width();
            const auto col = pixel_index % image.width();
            img[row][col] = image.at(pixel_index).index();
        }
        img.write(path);
    };

    // Write PNG to filesystem using the appropriate pixel type for mode
    try {
        if (mode == greyscale) {
            png::image<png::index_pixel_4> out{image.width(), image.height()};
            write_image(out);
        }
        else {
            png::image<png::index_pixel> out{image.width(), image.height()};
            write_image(out);
        }
    }
    catch (const std::exception &e) {
        return FormattableError{fmt::format("{}: save failed: {}", path.filename().c_str(), e.what())};
    }

    return {};
}

} // namespace porytiles2
