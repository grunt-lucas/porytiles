#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief An image loader that reads PNG files to create an Image with an index pixel type.
///
/// @details
/// This loader's implementation uses png++ wrapper for libpng to read PNG data and load it into an Image.
class PngIndexedImageLoader final {
  public:
    PngIndexedImageLoader() = default;

    [[nodiscard]] ChainableResult<std::unique_ptr<Image<IndexPixel>>>
    load_from_file(const std::filesystem::path &path) const;
};

} // namespace porytiles
