#pragma once

#include <filesystem>
#include <memory>

#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/infra/services/image_load_error.hpp"

namespace porytiles {

/**
 * @brief An image loader that reads PNG files to create an Image with an Rgba32 pixel type.
 *
 * @details
 * This loader's underlying implementation uses the png++ wrapper for libpng to read PNG data and load it into an
 * Image. However, these external library details are entirely encapsulated within the implementation. Users of the
 * Porytiles library need not concern themselves with png++ details.
 */
class PngRgbaImageLoader final {
  public:
    PngRgbaImageLoader() = default;

    [[nodiscard]] ChainableResult<std::unique_ptr<Image<Rgba32>>, ImageLoadError>
    load_from_file(const std::filesystem::path &path) const;
};

} // namespace porytiles
