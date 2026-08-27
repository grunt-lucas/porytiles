#pragma once

#include <filesystem>

#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

/// @brief A service interface that loads a fixed-length Palette from a given file.
///
/// @details
/// The FilePaletteLoader interface is file-format-agnostic. Different implementations can load from various palette
/// file formats (e.g., JASC, .gbapal, etc.).
class FilePaletteLoader {
  public:
    virtual ~FilePaletteLoader() = default;

    [[nodiscard]] virtual ChainableResult<Palette<Rgba32, palette::max_size>>
    load(const std::filesystem::path &path) const = 0;

    [[nodiscard]] virtual ChainableResult<Palette<Rgba32, palette::max_size>>
    load_with_wildcards(const std::filesystem::path &path) const = 0;
};

} // namespace porytiles
