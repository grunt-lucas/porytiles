#pragma once

#include <filesystem>
#include <memory>

#include "porytiles2/domain/model/valueobj/RgbaImage.hpp"
#include "porytiles2/domain/services/RgbaImageLoader.hpp"

namespace porytiles {

/**
 * @brief An implementation of RgbaImageLoader that reads PNG files.
 *
 * @details
 * This loader's underlying implementation uses the CImg image processing library to read PNG data
 * and load it into an RgbaImage. However, these external library details are entirely encapsulated
 * within the implementation. Users of the Porytiles library need not concern themselves with CImg
 * details.
 */
class PngRgbaImageLoader final : public RgbaImageLoader {
public:
  PngRgbaImageLoader() = default;

  [[nodiscard]] Result<std::unique_ptr<RgbaImage>>
  LoadFromFile(const std::filesystem::path &path) const override;
};

} // namespace porytiles