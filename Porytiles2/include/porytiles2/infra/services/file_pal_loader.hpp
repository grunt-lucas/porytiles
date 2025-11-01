#pragma once

#include <filesystem>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief A service interface that loads a Palette from a given file.
 *
 * @details
 * The FilePalLoader interface is file-format-agnostic. Different implementations can load from various pal file formats
 * (e.g., JASC, .gbapal, etc.).
 */
class FilePalLoader {
  public:
    virtual ~FilePalLoader() = default;

    [[nodiscard]] virtual Result<Palette<Rgba32>> load(const std::filesystem::path &path) const = 0;
};

} // namespace porytiles2
