#pragma once

#include <filesystem>

#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief A service interface that saves a fixed-length Palette to a given file.
///
/// @details
/// The FilePalSaver interface is file-format-agnostic. Different implementations can save to various pal file formats
/// (e.g., JASC, .gbapal, etc.).
class FilePalSaver {
  public:
    virtual ~FilePalSaver() = default;

    [[nodiscard]] virtual ChainableResult<void>
    save(const Palette<Rgba32, pal::max_size> &pal, const std::filesystem::path &path) const = 0;
};

} // namespace porytiles
