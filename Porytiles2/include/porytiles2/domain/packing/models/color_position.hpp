#pragma once

#include <cstddef>
#include <variant>

#include "porytiles2/domain/models/rgba32.hpp"

namespace porytiles2 {

/**
 * @brief Position state for a color that has not yet been assigned a palette slot.
 *
 * @details
 * During palette construction, colors start as Undetermined. Sequential fill assigns them Absolute positions.
 * Colors with Indirect links skip sequential fill and resolve later.
 *
 * The Undetermined/Absolute/Indirect state machine is inspired by the @c Position_in_palette enum from borytiles by
 * ishax-kos (https://github.com/ishax-kos/borytiles), specifically its @c palette.rs module. Borytiles introduced the
 * insight that palette slot assignment can be decoupled from palette construction by linking colors to other colors
 * rather than to absolute slot indices.
 *
 * @see https://github.com/ishax-kos/borytiles
 */
struct UndeterminedPosition {};

/**
 * @brief Position state for a color assigned to a specific palette slot.
 *
 * @details
 * After sequential fill or Indirect resolution, a color occupies a fixed slot in its palette.
 */
struct AbsolutePosition {
    std::size_t slot;
};

/**
 * @brief Position state for a color whose slot is determined by another color in another palette.
 *
 * @details
 * IndirectPosition says "my slot is wherever @c ref_color ends up in palette @c ref_pal_index." This is the key
 * insight from borytiles by ishax-kos (https://github.com/ishax-kos/borytiles): by referencing colors rather than
 * absolute slots, links remain valid even when sequential fill places non-shared colors at different positions than the
 * base palettes. Corresponds to the @c Indirect(Palette_index,Color_index) variant of borytiles'
 * @c Position_in_palette enum.
 *
 * Indirect chains are resolved after sequential fill by following @c ref_pal_index / @c ref_color until hitting an
 * AbsolutePosition. Cycle detection caps resolution at @c pal::num_pals iterations.
 */
struct IndirectPosition {
    std::size_t ref_pal_index;
    Rgba32 ref_color;
    std::size_t source_group_index;
};

/**
 * @brief The position state of a color during palette construction.
 *
 * @details
 * Colors transition through states during palette building:
 * - Start as UndeterminedPosition
 * - Indirect links set some to IndirectPosition (referencing colors in other palettes)
 * - Sequential fill converts remaining UndeterminedPosition to AbsolutePosition
 * - Resolution follows IndirectPosition chains to AbsolutePosition
 */
using ColorPosition = std::variant<UndeterminedPosition, AbsolutePosition, IndirectPosition>;

/**
 * @brief Instruction linking a source color to a reference color in another palette.
 *
 * @details
 * An IndirectLink says: "source_color in palette source_pal should occupy the same slot as ref_color in palette
 * ref_pal." During palette construction, this translates to setting source_color's position to
 * IndirectPosition{ref_pal, ref_color}. When resolved, both colors end up at the same slot index in their respective
 * palettes, enabling tile sharing.
 *
 * @invariant source_pal != ref_pal (links always cross palette boundaries)
 */
struct IndirectLink {
    /**
     * @brief The palette index containing the color to be linked.
     */
    std::size_t source_pal;

    /**
     * @brief The color in source_pal that should follow the reference.
     */
    Rgba32 source_color;

    /**
     * @brief The palette index containing the reference color.
     */
    std::size_t ref_pal;

    /**
     * @brief The color in ref_pal whose slot position should be followed.
     */
    Rgba32 ref_color;

    /**
     * @brief The shape group index that generated this link (for diagnostic tracing).
     */
    std::size_t source_group_index;
};

} // namespace porytiles2
