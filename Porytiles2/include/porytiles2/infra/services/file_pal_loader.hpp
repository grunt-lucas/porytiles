#pragma once

#include <filesystem>

#include "porytiles2/domain/model/rgba_pal.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief A service interface that loads an RgbaPal from a given file.
 *
 * @details
 * The FilePalLoader interface is file-format-agnostic. Different implementations can load from various pal file formats
 * (e.g., JASC, .gbapal, etc.).
 */
class FilePalLoader {
  public:
    virtual ~FilePalLoader() = default;

    [[nodiscard]] virtual Result<RgbaPal> load(std::filesystem::path &path) const = 0;
};

} // namespace porytiles2
