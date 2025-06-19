#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

#include <porytiles2/infra/png/png.hpp>
#include <porytiles2/infra/png/png_importer.hpp>

namespace porytiles {

class PngImporterImpl final : public PngImporter {
  public:
    PngImporterImpl() = default;

    [[nodiscard]] std::expected<std::unique_ptr<Png>, std::string>
    Read(const std::filesystem::path &path) const override;
};

} // namespace porytiles