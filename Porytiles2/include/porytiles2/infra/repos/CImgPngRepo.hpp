#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

#include "porytiles2/domain/entities/Png.hpp"
#include "porytiles2/domain/repos/PngRepo.hpp"

namespace porytiles {

class CImgPngRepo final : public PngRepo {
  public:
    CImgPngRepo() = default;

    [[nodiscard]] std::expected<std::unique_ptr<Png>, std::string>
    Read(const std::filesystem::path &path) const override;
};

} // namespace porytiles
