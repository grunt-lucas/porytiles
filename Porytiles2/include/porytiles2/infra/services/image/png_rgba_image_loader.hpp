#pragma once

#include <filesystem>
#include <memory>

#include "porytiles2/domain/model/valueobj/image.hpp"
#include "porytiles2/domain/model/valueobj/rgba32.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief An image loader that reads PNG files to create an Image with an Rgba32 pixel type.
 *
 * @details
 * This loader's underlying implementation uses the CImg image processing library to read PNG data
 * and load it into an Image. However, these external library details are entirely encapsulated
 * within the implementation. Users of the Porytiles library need not concern themselves with CImg
 * details.
 */
class PngRgbaImageLoader final {
  public:
    PngRgbaImageLoader() = default;

    [[nodiscard]] Result<std::unique_ptr<Image<Rgba32>>> load_from_file(const std::filesystem::path &path) const;
};

} // namespace porytiles2
