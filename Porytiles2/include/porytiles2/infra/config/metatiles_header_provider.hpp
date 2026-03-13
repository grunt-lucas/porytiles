#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

#include "gsl/pointers"

#include "porytiles2/infra/config/config_provider.hpp"
#include "porytiles2/infra/config/layer_value.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief A ConfigProvider that auto-detects metatile attribute size from metatiles.h declarations.
 *
 * @details
 * MetatilesHeaderProvider scans `src/data/tilesets/metatiles.h` for `gMetatileAttributes_` declaration lines and
 * infers the attribute byte size from the C type used:
 * - All `const u16` → 2 bytes (pokeemerald / pokeruby)
 * - All `const u32` → 4 bytes (pokefirered)
 * - Mixed types → invalid (error)
 * - No matching lines or missing file → not_provided (fall through to next provider)
 *
 * The file is read lazily on first access and the result is cached.
 */
class MetatilesHeaderProvider final : public ConfigProvider {
  public:
    /**
     * @brief Constructs a MetatilesHeaderProvider.
     *
     * @param project_root The root directory of the decomp project
     * @param format A pointer to the TextFormatter to use for error messages
     */
    explicit MetatilesHeaderProvider(std::filesystem::path project_root, gsl::not_null<const TextFormatter *> format)
        : project_root_{std::move(project_root)}, format_{format}
    {
    }

    [[nodiscard]] std::string name() const override;

    [[nodiscard]] LayerValue<std::size_t>
    metatile_attr_size(ConfigScopeType type, const std::string &scope) const override;

  private:
    std::filesystem::path project_root_;
    const TextFormatter *format_;
    mutable std::optional<LayerValue<std::size_t>> cached_result_;
};

} // namespace porytiles2
