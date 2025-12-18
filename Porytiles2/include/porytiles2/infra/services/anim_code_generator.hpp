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

  private:
    /**
     * @brief Generates INCBIN statements for all frames of an animation.
     */
    [[nodiscard]] std::string generate_incbin_statements(
        const std::string &tileset_name,
        const std::filesystem::path &tileset_path_from_project_root,
        const std::string &anim_name,
        const AnimationParams &params) const;

    /**
     * @brief Generates the frame pointer array for an animation.
     */
    [[nodiscard]] std::string generate_frame_array(
        const std::string &tileset_name, const std::string &anim_name, const AnimationParams &params) const;

    /**
     * @brief Generates the QueueAnimTiles function for an animation.
     */
    [[nodiscard]] std::string generate_queue_function(
        const std::string &tileset_name, const std::string &anim_name, const AnimationParams &params) const;

    /**
     * @brief Generates the main TilesetAnim driver function.
     */
    [[nodiscard]] std::string generate_driver_function(
        const std::string &tileset_name, const std::map<std::string, AnimationParams> &animations) const;

    /**
     * @brief Generates the InitTilesetAnim function.
     */
    [[nodiscard]] std::string generate_init_function(
        const std::string &tileset_name,
        const std::map<std::string, AnimationParams> &animations,
        bool is_primary) const;

    /**
     * @brief Converts an animation name to PascalCase for use in C identifiers.
     */
    [[nodiscard]] std::string to_pascal_case(const std::string &name) const;

    /**
     * @brief Finds the maximum frame index referenced in the frames array.
     */
    [[nodiscard]] std::size_t find_max_frame_index(const std::vector<std::size_t> &frames) const;
};

} // namespace porytiles2