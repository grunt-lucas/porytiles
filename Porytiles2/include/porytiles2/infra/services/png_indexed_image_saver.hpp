#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief An image saver that saves PNG files from an Image with an index pixel type.
 *
 * @details
 * This loader's implementation uses png++ wrapper for libpng to save the Image to a PNG.
 */
class PngIndexedImageSaver final {
  public:
    PngIndexedImageSaver() = default;

    [[nodiscard]] Result<void> save_to_file(const Image<std::uint8_t> &image, const std::filesystem::path &path) const;
};

} // namespace porytiles2
