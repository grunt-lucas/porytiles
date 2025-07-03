#pragma once

#include <expected>
#include <filesystem>
#include <memory>

#include "porytiles2/domain/model/valueobj/RgbaImage.hpp"
#include "porytiles2/domain/repos/RgbaImageRepo.hpp"

namespace porytiles {

/**
 * @brief An implementation of RgbaImageRepo that reads a PNG using the CImg
 * library.
 */
class CImgPngRgbaImageRepo final : public RgbaImageRepo {
public:
  CImgPngRgbaImageRepo() = default;

  [[nodiscard]] Result<std::unique_ptr<RgbaImage>>
  Read(const std::filesystem::path &path) const override;
};

} // namespace porytiles
