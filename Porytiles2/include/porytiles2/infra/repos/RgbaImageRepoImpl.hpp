#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

#include "porytiles2/domain/entities/RgbaImage.hpp"
#include "porytiles2/domain/repos/RgbaImageRepo.hpp"

namespace porytiles {

class RgbaImageRepoImpl final : public RgbaImageRepo {
public:
  RgbaImageRepoImpl() = default;

  [[nodiscard]] Result<std::unique_ptr<RgbaImage>>
  Read(const std::filesystem::path &path) const override;
};

} // namespace porytiles
