#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

#include <porytiles2/png/png_importer.hpp>

namespace porytiles {

class CImgPngImporter final : public PngImporter {
  public:
    CImgPngImporter() = default;

    [[nodiscard]] std::expected<std::unique_ptr<Png>, std::string>
    Read(const std::filesystem::path &path) const override;
};

} // namespace porytiles