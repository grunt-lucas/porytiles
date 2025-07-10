#pragma once

#include <filesystem>

#include "porytiles2/domain/model/valueobj/RgbaImage.hpp"
#include "porytiles2/domain/services/RgbaImageSaver.hpp"

namespace porytiles2 {

/**
 * @brief An implementation of RgbaImageSaver that writes PNG files.
 *
 * @details
 * This saver's underlying implementation uses the CImg image processing library to write PNG data
 * from an RgbaImage. However, these external library details are entirely encapsulated within the
 * implementation. Users of the Porytiles library need not concern themselves with CImg details.
 */
class PngRgbaImageSaver final : public RgbaImageSaver {
public:
  PngRgbaImageSaver() = default;

  [[nodiscard]] Result<void> save_to_file(const RgbaImage &image,
                                          const std::filesystem::path &path) const override;
};

} // namespace porytiles2