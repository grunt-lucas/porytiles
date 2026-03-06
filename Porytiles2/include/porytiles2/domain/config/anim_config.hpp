#pragma once

#include <optional>
#include <string>
#include <vector>

#include "porytiles2/domain/config/anim_key_frame_resolution_strategy.hpp"
#include "porytiles2/domain/config/anim_pal_resolution_strategy.hpp"
#include "porytiles2/domain/config/frame_linking.hpp"

namespace porytiles2 {

/**
 * @brief Per-animation configuration for animation decompilation.
 *
 * @details
 * Holds palette resolution configuration, key frame resolution strategy, and a frame linking mode for a single
 * animation. Palette resolution follows a three-tier cascade (most specific wins):
 *
 * 1. **Per-tile** (@c per_tile_pal_resolution_strategies) — most specific. When non-empty, its length must exactly
 *    match the animation's @c tile_count. Each entry can be @c std::nullopt to fall through to the next tier.
 * 2. **Per-anim** (@c pal_resolution_strategy) — middle tier. A single strategy applied to all subtiles of this
 *    animation that are not overridden by a per-tile entry.
 * 3. **Global** (@c palette_resolution_strategy from DomainConfig) — least specific fallback.
 *
 * Key frame resolution also supports a two-tier cascade: per-anim @c key_frame_resolution_strategy wins, otherwise
 * falls back to @c key_frame_resolution_strategy from DomainConfig.
 */
struct AnimConfig {
    std::string anim_name;
    std::optional<AnimPalResolutionStrategy> pal_resolution_strategy{std::nullopt};
    std::vector<std::optional<AnimPalResolutionStrategy>> per_tile_pal_resolution_strategies;
    std::optional<AnimKeyFrameResolutionStrategy> key_frame_resolution_strategy{std::nullopt};
    FrameLinking linking{FrameLinking::automatic};
};

} // namespace porytiles2
