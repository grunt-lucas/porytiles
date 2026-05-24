#pragma once

#include <string>
#include <vector>

#include "porytiles/domain/config/anim_key_frame_resolution_strategy.hpp"
#include "porytiles/domain/config/anim_multi_pal_subtile_resolution_strategy.hpp"
#include "porytiles/domain/config/anim_pal_resolution_strategy.hpp"
#include "porytiles/domain/config/frame_linking.hpp"
#include "porytiles/xcut/config/config_pod_field.hpp"

namespace porytiles {

/**
 * @brief Per-animation configuration override for animation decompilation.
 *
 * @details
 * Holds palette resolution configuration, key frame resolution strategy, and a frame linking mode for a single
 * animation. Palette resolution follows a three-tier cascade (most specific wins):
 *
 * 1. **Per-tile** (@c per_tile_pal_resolution_strategies): most specific. When non-empty, its length must exactly
 *    match the animation's @c tile_count. Each entry can be empty (no value) to fall through to the next tier.
 * 2. **Per-anim** (@c pal_resolution_strategy): middle tier. A single strategy applied to all subtiles of this
 *    animation that are not overridden by a per-tile entry.
 * 3. **Global** (@c palette_resolution_strategy from DomainConfig): least specific fallback.
 *
 * Key frame resolution also supports a two-tier cascade: per-anim @c key_frame_resolution_strategy wins, otherwise
 * falls back to @c key_frame_resolution_strategy from DomainConfig.
 *
 * Multi-palette subtile resolution follows the same two-tier cascade: per-anim
 * @c multi_pal_subtile_resolution_strategy wins, otherwise falls back to
 * @c multi_pal_subtile_resolution_strategy from DomainConfig.
 *
 * Frame linking also follows the same two-tier cascade: per-anim wins, otherwise falls back to the global setting.
 *
 * Override fields use @c ConfigPODField<T> instead of @c std::optional<T> to carry provider-specific source metadata
 * (source_key, canonical_name) set at parse time, enabling @c ConfigValue::derive() to construct properly attributed
 * child values without hardcoding provider formats.
 */
struct PerAnimOverride {
    std::string anim_name;
    ConfigPODField<AnimPalResolutionStrategy> pal_resolution_strategy;
    std::vector<ConfigPODField<AnimPalResolutionStrategy>> per_tile_pal_resolution_strategies;
    ConfigPODField<AnimKeyFrameResolutionStrategy> key_frame_resolution_strategy;
    ConfigPODField<AnimMultiPalSubtileResolutionStrategy> multi_pal_subtile_resolution_strategy;
    ConfigPODField<FrameLinking> linking;
};

} // namespace porytiles
