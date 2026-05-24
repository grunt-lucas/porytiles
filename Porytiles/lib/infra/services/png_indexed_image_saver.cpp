#include "porytiles/infra/services/png_indexed_image_saver.hpp"

#include <filesystem>
#include <format>
#include <type_traits>

#include "png++/png.hpp"

#include "porytiles/domain/config/tiles_pal_mode.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

ChainableResult<void> PngIndexedImageSaver::save_to_file(
    const Image<IndexPixel> &image, const std::filesystem::path &path, TilesPalMode mode) const
{
    using enum TilesPalMode;

    const auto greyscale_pal = standard_greyscale_pal();

    // Bail if given path exists already and isn't a file (i.e. it's a directory)
    if (exists(path) && !is_regular_file(path)) {
        return FormattableError{std::format("{}: exists but is not a file", path.filename().c_str())};
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

    // Generic lambda to write indexed PNG with any pixel type.
    // For 4-bit PNGs, we explicitly extract the lower 4 bits (color_index) to be self-documenting.
    // For 8-bit PNGs, we use the full index value which may encode both palette and color index.
    auto write_image = [&]<typename PixelType>(png::image<PixelType> &img) {
        img.set_palette(png_pal);
        for (std::size_t pixel_index = 0; pixel_index < image.size(); pixel_index++) {
            const auto row = pixel_index / image.width();
            const auto col = pixel_index % image.width();
            if constexpr (std::is_same_v<PixelType, png::index_pixel_4>) {
                // 4-bit PNG: extract only the lower 4 bits (color index within palette)
                img[row][col] = image.at(pixel_index).color_index();
            }
            else {
                // 8-bit PNG: preserve full value (may include palette index in upper 4 bits for true-color mode)
                img[row][col] = image.at(pixel_index).index();
            }
        }
        img.write(path);
    };

    // Write PNG to filesystem using the appropriate pixel type for mode
    try {
        // If internal palette is size 16 or less, use 4-bit PNG palette
        if (palette_to_use.size() <= 16) {
            png::image<png::index_pixel_4> out{image.width(), image.height()};
            write_image(out);
        }
        else {
            png::image<png::index_pixel> out{image.width(), image.height()};
            write_image(out);
        }
    }
    catch (const std::exception &e) {
        return FormattableError{std::format("{}: save failed: {}", path.filename().c_str(), e.what())};
    }

    return {};
}

} // namespace porytiles
