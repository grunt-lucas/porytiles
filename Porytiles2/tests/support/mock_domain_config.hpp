#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/config/anim_key_frame_resolution_strategy.hpp"
#include "porytiles2/domain/config/anim_pal_resolution_strategy.hpp"
#include "porytiles2/domain/config/anim_pal_resolution_strategy_overrides.hpp"
#include "porytiles2/domain/config/artifact_edit_mode.hpp"
#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/domain/config/tiles_pal_mode.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/packing/models/palette_hint.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/config_scope_type.hpp"
#include "porytiles2/xcut/config/config_value.hpp"

namespace porytiles2 {

/**
 * @brief Shared reusable MockDomainConfig for test suites.
 *
 * @details
 * Implements all 17 pure virtual _raw methods with sensible defaults. Animation-related and extrinsic transparency
 * fields are exposed as public members for per-test customization.
 */
class MockDomainConfig : public DomainConfig {
  public:
    // Customizable fields for animation-related config
    AnimPalResolutionStrategy anim_pal_strategy = AnimPalResolutionStrategy::internal_png_pal;
    AnimPalResolutionStrategyOverrides anim_pal_overrides{};
    AnimKeyFrameResolutionStrategy anim_key_frame_strategy = AnimKeyFrameResolutionStrategy::error;
    Rgba32 transparency_color{0, 0, 0, 255};

  protected:
    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    num_tiles_in_primary_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{std::size_t{512}, "num_tiles_in_primary", "num_tiles_in_primary", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    num_tiles_total_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{std::size_t{1024}, "num_tiles_total", "num_tiles_total", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    num_metatiles_in_primary_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{std::size_t{512}, "num_metatiles_in_primary", "num_metatiles_in_primary", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    num_metatiles_total_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{std::size_t{1024}, "num_metatiles_total", "num_metatiles_total", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    num_pals_in_primary_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{std::size_t{6}, "num_pals_in_primary", "num_pals_in_primary", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    num_pals_total_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{std::size_t{13}, "num_pals_total", "num_pals_total", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    max_map_data_size_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{std::size_t{10000}, "max_map_data_size", "max_map_data_size", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    num_tiles_per_metatile_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{std::size_t{8}, "num_tiles_per_metatile", "num_tiles_per_metatile", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<Rgba32>>
    extrinsic_transparency_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{transparency_color, "extrinsic_transparency", "extrinsic_transparency", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<ArtifactEditMode>>
    tiles_edit_mode_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{ArtifactEditMode::optimize, "tiles_edit_mode", "tiles_edit_mode", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<ArtifactEditMode>>
    pals_edit_mode_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{ArtifactEditMode::optimize, "pals_edit_mode", "pals_edit_mode", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<bool>>
    pal_hints_enabled_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{false, "pal_hints_enabled", "pal_hints_enabled", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::vector<PaletteHint>>>
    pal_hints_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{std::vector<PaletteHint>{}, "pal_hints", "pal_hints", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<TilesPalMode>>
    tiles_pal_mode_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{TilesPalMode::true_color, "tiles_pal_mode", "tiles_pal_mode", "mock", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<AnimPalResolutionStrategy>>
    global_anim_pal_resolution_strategy_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{
            anim_pal_strategy,
            "global_anim_pal_resolution_strategy",
            "global_anim_pal_resolution_strategy",
            "mock",
            {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<AnimPalResolutionStrategyOverrides>>
    anim_pal_resolution_strategy_overrides_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{
            anim_pal_overrides,
            "anim_pal_resolution_strategy_overrides",
            "anim_pal_resolution_strategy_overrides",
            "mock",
            {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<AnimKeyFrameResolutionStrategy>>
    anim_key_frame_resolution_strategy_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue{
            anim_key_frame_strategy,
            "anim_key_frame_resolution_strategy",
            "anim_key_frame_resolution_strategy",
            "mock",
            {}};
    }
};

} // namespace porytiles2
