#pragma once

#include <filesystem>

#include "porytiles2/domain/model/rgba_pal.hpp"
#include "porytiles2/templates/result.hpp"

namespace porytiles2 {

/**
 * @brief A service interface that saves an RgbaPal to a given file.
 *
 * @details
 * The FilePalSaver interface is file-format-agnostic. Different implementations can save to various pal file formats
 * (e.g., JASC, .gbapal, etc.).
 */
class FilePalSaver {
  public:
    virtual ~FilePalSaver() = default;

    [[nodiscard]] virtual Result<void> save(const RgbaPal &pal, const std::filesystem::path &path) const = 0;
};

} // namespace porytiles2
