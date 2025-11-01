#pragma once

#include <filesystem>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief A service interface that saves a Palette to a given file.
 *
 * @details
 * The FilePalSaver interface is file-format-agnostic. Different implementations can save to various pal file formats
 * (e.g., JASC, .gbapal, etc.).
 */
class FilePalSaver {
  public:
    virtual ~FilePalSaver() = default;

    [[nodiscard]] virtual ChainableResult<void>
    save(const Palette<Rgba32> &pal, const std::filesystem::path &path) const = 0;
};

} // namespace porytiles2
