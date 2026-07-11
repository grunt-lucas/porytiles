#pragma once

#include <filesystem>

#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief An image saver that saves PNG files from an Image with an Rgba32 pixel type.
///
/// @details
/// This saver's underlying implementation uses the png++ wrapper for libpng to write PNG data from an rgba Image.
/// However, these external library details are entirely encapsulated within the implementation. Users of the Porytiles
/// library need not concern themselves with png++ details.
class PngRgbaImageSaver {
  public:
    PngRgbaImageSaver() = default;
    virtual ~PngRgbaImageSaver() = default;

    [[nodiscard]] virtual ChainableResult<void>
    save_to_file(const Image<Rgba32> &image, const std::filesystem::path &path) const;
};

} // namespace porytiles
