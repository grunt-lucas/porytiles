#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

#include <porytiles2/png/png.hpp>

namespace porytiles {

class PngImporter {
  public:
    virtual ~PngImporter() = default;

    [[nodiscard]] virtual std::expected<std::unique_ptr<Png>, std::string>
    Read(const std::filesystem::path &path) const = 0;
};

} // namespace porytiles