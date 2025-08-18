#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief An image loader that reads PNG files to create an Image with an index pixel type.
 *
 * @details
 * This loader's implementation uses png++ wrapper for libpng to read PNG data and load it into an Image.
 */
class PngIndexedImageLoader final {
  public:
    PngIndexedImageLoader() = default;

    [[nodiscard]] Result<std::unique_ptr<Image<std::uint8_t>>> load_from_file(const std::filesystem::path &path) const;
};

} // namespace porytiles2
