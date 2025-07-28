#pragma once

#include <filesystem>

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/rgba32.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief An image saver that saves PNG files from an Image with an Rgba32 pixel type.
 *
 * @details
 * This saver's underlying implementation uses the CImg image processing library to write PNG data
 * from an rgba Image. However, these external library details are entirely encapsulated within the
 * implementation. Users of the Porytiles library need not concern themselves with CImg details.
 */
class PngRgbaImageSaver final {
  public:
    PngRgbaImageSaver() = default;

    [[nodiscard]] Result<void> save_to_file(const Image<Rgba32> &image, const std::filesystem::path &path) const;
};

} // namespace porytiles2
