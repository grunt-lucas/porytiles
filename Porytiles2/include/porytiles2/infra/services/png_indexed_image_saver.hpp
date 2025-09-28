#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/index_pixel.hpp"
#include "porytiles2/infra/config/tiles_pal_mode.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief An image saver that saves PNG files from an Image with an index pixel type.
 *
 * @details
 * This loader's implementation uses png++ wrapper for libpng to save the Image to a PNG.
 */
class PngIndexedImageSaver {
  public:
    PngIndexedImageSaver() = default;
    virtual ~PngIndexedImageSaver() = default;

    [[nodiscard]] virtual ChainableResult<void>
    save_to_file(const Image<IndexPixel> &image, const std::filesystem::path &path, TilesPalMode mode) const;
};

} // namespace porytiles2
