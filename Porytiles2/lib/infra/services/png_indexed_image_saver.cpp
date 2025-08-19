#include "porytiles2/infra/services/png_indexed_image_saver.hpp"

#include <array>
#include <filesystem>

#include "fmt/format.h"
#include "png++/png.hpp"

#include "porytiles2/domain/model/index_pixel.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/infra/config/tiles_pal_mode.hpp"
#include "porytiles2/templates/panic.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

Result<void> PngIndexedImageSaver::save_to_file(
    const Image<IndexPixel> &image, const std::filesystem::path &path, TilesPalMode mode) const {
    using enum TilesPalMode;

    const std::vector greyscale_pal = {
        Rgba32{0, 0, 0, 255},
        Rgba32{16, 16, 16, 255},
        Rgba32{32, 32, 32, 255},
        Rgba32{48, 48, 48, 255},
        Rgba32{64, 64, 64, 255},
        Rgba32{80, 80, 80, 255},
        Rgba32{96, 96, 96, 255},
        Rgba32{112, 112, 112, 255},
        Rgba32{128, 128, 128, 255},
        Rgba32{144, 144, 144, 255},
        Rgba32{160, 160, 160, 255},
        Rgba32{176, 176, 176, 255},
        Rgba32{192, 192, 192, 255},
        Rgba32{208, 208, 208, 255},
        Rgba32{224, 224, 224, 255},
        Rgba32{240, 240, 240, 255}};

    png::palette png_pal{0};
    png::image<png::index_pixel> out{image.width(), image.height()};

    // Bail if given path exists already and isn't a file (i.e. it's a directory)
    if (exists(path) && !is_regular_file(path)) {
        return std::unexpected{fmt::format("exists but is not a file: {}", path.string())};
    }

    // Determine which palette to use
    std::vector<Rgba32> palette_to_use;
    if (mode == true_color && image.palette().has_value()) {
        palette_to_use = image.palette().value();
    } else {
        // Use greyscale palette for greyscale mode OR when true_color mode but no image palette exists
        palette_to_use = greyscale_pal;
    }

    // Set up PNG palette
    for (const auto &color : palette_to_use) {
        png_pal.emplace_back(color.red(), color.green(), color.blue());
    }
    out.set_palette(png_pal);

    // Write data to PNG buffer
    for (unsigned int pixel_index = 0; pixel_index < image.size(); pixel_index++) {
        const auto row = pixel_index / image.width();
        const auto col = pixel_index % image.width();
        out[row][col] = image.at(pixel_index).index();
    }

    // Write PNG to filesystem
    try {
        out.write(path);
    } catch (const std::exception &e) {
        return std::unexpected{fmt::format("failed to save indexed PNG to {}: {}", path.string(), e.what())};
    }

    return {};
}

} // namespace porytiles2
