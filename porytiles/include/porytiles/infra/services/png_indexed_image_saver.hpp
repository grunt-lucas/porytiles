#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

#include "porytiles/domain/config/tiles_palette_mode.hpp"
#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief An image saver that saves PNG files from an Image with an index pixel type.
///
/// @details
/// This loader's implementation uses png++ wrapper for libpng to save the Image to a PNG.
class PngIndexedImageSaver {
  public:
    PngIndexedImageSaver() = default;
    virtual ~PngIndexedImageSaver() = default;

    [[nodiscard]] virtual ChainableResult<void>
    save_to_file(const Image<IndexPixel> &image, const std::filesystem::path &path, TilesPaletteMode mode) const;
};

} // namespace porytiles
