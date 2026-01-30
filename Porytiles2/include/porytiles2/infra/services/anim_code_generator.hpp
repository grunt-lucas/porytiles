#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "porytiles2/domain/models/animation_params.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Generates C header code for tileset animations.
 *
 * @details
 * AnimCodeGenerator produces the `generated_anim_code.h` header file that contains all the C code needed for tileset
 * animations to work in a pokeemerald project. This includes:
 * - INCBIN statements for each animation frame's .4bpp file
 * - Frame pointer arrays that define the animation sequence
 * - QueueAnimTiles functions for each animation
 * - A main TilesetAnim driver function that calls all Queue functions at appropriate intervals
 * - An InitTilesetAnim function to initialize the animation system
 *
 * The generated code follows the pattern used by vanilla pokeemerald animations, making it compatible with the existing
 * tileset animation infrastructure.
 *
 * Usage:
 * @code
 * AnimCodeGenerator generator;
 * auto result = generator.generate(
 *     "PorytilesAnimExample",
 *     "data/tilesets/primary/porytiles_anim_example",
 *     animations,
 *     TilesetType::primary);
 * @endcode
 */
class AnimCodeGenerator {
  public:
    /**
     * @brief Generates the complete `generated_anim_code.h` content.
     *
     * @details
     * Produces a complete C header file containing all animation code for a tileset. The generated code includes proper
     * include guards, INCBIN macro definitions (if not already defined), and all animation functions.
     *
     * @param tileset_name The name of the tileset (e.g., "PorytilesAnimExample")
     * @param tileset_path_from_project_root Relative path from project root to tileset directory
     * @param animations Map of animation names to their parameters
     * @param is_primary True if this is a primary tileset, false for secondary
     * @return The generated C header code as a string, or error if generation fails
     */
    [[nodiscard]] ChainableResult<std::string> generate(
        const std::string &tileset_name,
        const std::filesystem::path &tileset_path_from_project_root,
        const std::map<std::string, AnimationParams> &animations,
        bool is_primary) const;
};

} // namespace porytiles2