#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>

#include "gsl/pointers"

#include "porytiles/infra/config/layer_value.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

/// @brief A standalone detector that infers the metatile attribute size from metatiles.h declarations.
///
/// @details
/// MetatilesHeaderProvider scans `src/data/tilesets/metatiles.h` for `gMetatileAttributes_` declaration lines and
/// infers the attribute byte size from the C type used:
/// - All `const u16` -> 2 bytes (emerald-family layouts)
/// - All `const u32` -> 4 bytes (firered-family layouts)
/// - Mixed types -> invalid (error)
/// - No matching lines or missing file -> not_provided
///
/// This is the sole authoritative source for the attribute byte width; the schema resolver consumes it directly
/// (there is no user-facing size knob). It returns a LayerValue so the three states (valid / invalid / not_provided)
/// stay distinguishable: the resolver treats not_provided as the 2-byte default and surfaces invalid as an error.
///
/// The file is read lazily on first access and the result is cached.
class MetatilesHeaderProvider final {
    // TODO: this class is an oddball. It's in infra/config with a *Provider moniker, but it's not actually a
    // ConfigProvider. It's used by MetatileAttributeConfigProvider, which is a real provider. It's also used by
    // TilesetAttributeSchemaResolver. So this class really feels like in belongs in infra/services, except it has a
    // LayerValue return type on detect, which makes in an infra/config candidate. I think this is a smell.
  public:
    /// @brief Constructs a MetatilesHeaderProvider.
    ///
    /// @param project_root The root directory of the decomp project
    /// @param format A pointer to the TextFormatter to use for error messages
    explicit MetatilesHeaderProvider(std::filesystem::path project_root, gsl::not_null<const TextFormatter *> format)
        : project_root_{std::move(project_root)}, format_{format}
    {
    }

    /// @brief Detects the project-global metatile attribute byte size from metatiles.h.
    ///
    /// @return A LayerValue holding the detected size (2 or 4), invalid on mixed u16/u32 declarations, or
    /// not_provided when the header is missing or has no attribute declarations
    [[nodiscard]] LayerValue<std::size_t> detect() const;

  private:
    std::filesystem::path project_root_;
    const TextFormatter *format_;
    mutable std::optional<LayerValue<std::size_t>> cached_result_;
};

} // namespace porytiles
