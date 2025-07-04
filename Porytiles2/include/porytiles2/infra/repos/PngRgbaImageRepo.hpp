#pragma once

#include <expected>
#include <filesystem>
#include <memory>

#include "porytiles2/domain/model/valueobj/RgbaImage.hpp"
#include "porytiles2/domain/repos/RgbaImageRepo.hpp"

namespace porytiles {

/**
 * @brief An implementation of RgbaImageRepo that reads from a PNG backing store.
 *
 * @details
 * This repo's underlying implementation uses the CImg image processing library to read the PNG data
 * and load it into the RgbaImage. However, these external library details are entirely encapsulated
 * within the implementation. Users of the Porytiles library need not concern themselves with CImg
 * details.
 */
class PngRgbaImageRepo final : public RgbaImageRepo {
public:
  PngRgbaImageRepo() = default;

  [[nodiscard]] Result<std::unique_ptr<RgbaImage>>
  Read(const std::filesystem::path &path) const override;
};

} // namespace porytiles
