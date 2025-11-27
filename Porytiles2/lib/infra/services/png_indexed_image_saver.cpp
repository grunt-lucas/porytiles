#include "porytiles2/infra/services/png_indexed_image_saver.hpp"

#include <array>
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
     * TODO: make this configurable. Current pal has black as index 0, which matches OG porytiles as well as vanilla
     * game tilesets. But users might want to use this greyscale pal. Or they might want to use the true-color mode.
     */
    // const std::vector greyscale_pal = {
    //     Rgba32{255, 255, 255, 255},
    //     Rgba32{238, 238, 238, 255},
    //     Rgba32{222, 222, 222, 255},
    //     Rgba32{205, 205, 205, 255},
    //     Rgba32{189, 189, 189, 255},
    //     Rgba32{172, 172, 172, 255},
    //     Rgba32{156, 156, 156, 255},
    //     Rgba32{139, 139, 139, 255},
    //     Rgba32{115, 115, 115, 255},
    //     Rgba32{98, 98, 98, 255},
    //     Rgba32{82, 82, 82, 255},
    //     Rgba32{65, 65, 65, 255},
    //     Rgba32{49, 49, 49, 255},
    //     Rgba32{32, 32, 32, 255},
    //     Rgba32{16, 16, 16, 255},
    //     Rgba32{0, 0, 0, 255},
    // };

    const std::vector greyscale_pal = {
        Rgba32{255, 255, 255, 255},
        Rgba32{238, 238, 238, 255},
        Rgba32{222, 222, 222, 255},
        Rgba32{205, 205, 205, 255},
        Rgba32{189, 189, 189, 255},
        Rgba32{172, 172, 172, 255},
        Rgba32{156, 156, 156, 255},
        Rgba32{139, 139, 139, 255},
        Rgba32{115, 115, 115, 255},
        Rgba32{98, 98, 98, 255},
        Rgba32{82, 82, 82, 255},
        Rgba32{65, 65, 65, 255},
        Rgba32{49, 49, 49, 255},
        Rgba32{32, 32, 32, 255},
        Rgba32{16, 16, 16, 255},
        Rgba32{0, 0, 0, 255},
    };

    png::palette png_pal{0};
    // TODO: this is currently hardcoded to 4-bit pal
    // once we support different TilesPalModes, we'll need to dynamically adjust this
    png::image<png::index_pixel_4> out{image.width(), image.height()};

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
    }
    catch (const std::exception &e) {
        return FormattableError{fmt::format("{}: save failed: {}", path.filename().c_str(), e.what())};
    }

    return {};
}

} // namespace porytiles2
