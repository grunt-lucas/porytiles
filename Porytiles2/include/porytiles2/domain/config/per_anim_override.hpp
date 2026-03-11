#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/config/anim_key_frame_resolution_strategy.hpp"
#include "porytiles2/domain/config/anim_multi_pal_subtile_resolution_strategy.hpp"
#include "porytiles2/domain/config/anim_pal_resolution_strategy.hpp"
#include "porytiles2/domain/config/frame_linking.hpp"
#include "porytiles2/xcut/config/config_override.hpp"

namespace porytiles2 {

/**
 * @brief Per-animation configuration override for animation decompilation.
 *
 * @details
 * Holds palette resolution configuration, key frame resolution strategy, and a frame linking mode for a single
 * animation. Palette resolution follows a three-tier cascade (most specific wins):
 *
 * 1. **Per-tile** (@c per_tile_pal_resolution_strategies) — most specific. When non-empty, its length must exactly
 *    match the animation's @c tile_count. Each entry can be empty (no value) to fall through to the next tier.
 * 2. **Per-anim** (@c pal_resolution_strategy) — middle tier. A single strategy applied to all subtiles of this
 *    animation that are not overridden by a per-tile entry.
 * 3. **Global** (@c palette_resolution_strategy from DomainConfig) — least specific fallback.
 *
 * Key frame resolution also supports a two-tier cascade: per-anim @c key_frame_resolution_strategy wins, otherwise
 * falls back to @c key_frame_resolution_strategy from DomainConfig.
 *
 * Multi-palette subtile resolution follows the same two-tier cascade: per-anim
 * @c multi_pal_subtile_resolution_strategy wins, otherwise falls back to
 * @c multi_pal_subtile_resolution_strategy from DomainConfig.
 *
 * Override fields use @c ConfigOverride<T> instead of @c std::optional<T> to carry provider-specific source metadata
 * (source_key, canonical_name) set at parse time, enabling @c ConfigValue::derive() to construct properly attributed
 * child values without hardcoding provider formats.
 */
struct PerAnimOverride {
    std::string anim_name;
    ConfigOverride<AnimPalResolutionStrategy> pal_resolution_strategy;
    std::vector<ConfigOverride<AnimPalResolutionStrategy>> per_tile_pal_resolution_strategies;
    ConfigOverride<AnimKeyFrameResolutionStrategy> key_frame_resolution_strategy;
    ConfigOverride<AnimMultiPalSubtileResolutionStrategy> multi_pal_subtile_resolution_strategy;
    FrameLinking linking{FrameLinking::automatic};
};

} // namespace porytiles2
