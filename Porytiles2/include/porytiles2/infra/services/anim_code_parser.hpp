#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "porytiles2/domain/models/animation_params.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Parses C code to extract tileset animation parameters.
 *
 * @details
 * AnimCodeParser extracts AnimationParams from C source files. It supports two parsing modes:
 *
 * 1. **Generated header parsing**: Parses our own `generated_anim_code.h` format produced by AnimCodeGenerator. This is
 *    used for subsequent imports and compiles after the tileset has been onboarded to Porytiles management.
 *
 * 2. **Vanilla parsing**: Parses the original `tileset_anims.c` patterns used in pokeemerald. This is used for
 *    first-time imports when the tileset hasn't yet been onboarded to Porytiles.
 *
 * The parser extracts:
 * - tile_offset from TILE_OFFSET_4BPP(X) in QueueAnimTiles functions
 * - tile_count from X * TILE_SIZE_4BPP
 * - frame_factor from timer % X in driver functions
 * - frame_offset from timer % X == Y conditions
 * - frames array from frame pointer array definitions (order matters)
 *
 * Note: Animation frame PNG files are loaded separately by the artifact reader; this parser only extracts the
 * configuration parameters embedded in C code.
 */
class AnimCodeParser {
  public:
    /**
     * @brief Parses a Porytiles-generated animation header file.
     *
     * @details
     * Parses `generated_anim_code.h` files created by AnimCodeGenerator. Extracts all animation parameters including
     * tile_offset and tile_count which are embedded in the QueueAnimTiles functions.
     *
     * @param header_path Path to the generated_anim_code.h file
     * @return Map of animation names to their parsed parameters, or error
     */
    [[nodiscard]] ChainableResult<std::map<std::string, AnimationParams>>
    parse_generated_header(const std::filesystem::path &header_path) const;

    /**
     * @brief Parses a Porytiles-generated animation header from a string.
     *
     * @details
     * Same as parse_generated_header but takes the file content as a string. Useful for testing.
     *
     * @param content The header file content
     * @return Map of animation names to their parsed parameters, or error
     */
    [[nodiscard]] ChainableResult<std::map<std::string, AnimationParams>>
    parse_generated_header_content(const std::string &content) const;

    /**
     * @brief Parses vanilla pokeemerald tileset animation code.
     *
     * @details
     * Parses the original tileset_anims.c patterns used in pokeemerald for first-time imports. This handles the
     * various animation patterns found in vanilla code including simple animations and VDests patterns.
     *
     * Note: VDests patterns are currently detected but not fully supported; they will be parsed with default
     * parameters and a warning may be logged.
     *
     * @param anims_c_path Path to the tileset_anims.c file
     * @param tileset_name The name of the tileset to extract animations for
     * @return Map of animation names to their parsed parameters, or error
     */
    [[nodiscard]] ChainableResult<std::map<std::string, AnimationParams>>
    parse_vanilla_anims(const std::filesystem::path &anims_c_path, const std::string &tileset_name) const;

  private:
    /**
     * @brief Extracts animation names from INCBIN statements in the content.
     */
    [[nodiscard]] std::vector<std::string> extract_animation_names(const std::string &content) const;

    /**
     * @brief Parses tile_offset from a QueueAnimTiles function.
     */
    [[nodiscard]] std::optional<std::size_t>
    parse_tile_offset(const std::string &content, const std::string &anim_name) const;

    /**
     * @brief Parses tile_count from a QueueAnimTiles function.
     */
    [[nodiscard]] std::optional<std::size_t>
    parse_tile_count(const std::string &content, const std::string &anim_name) const;

    /**
     * @brief Parses frame_factor and frame_offset from driver function.
     */
    [[nodiscard]] std::pair<std::size_t, std::size_t>
    parse_frame_timing(const std::string &content, const std::string &anim_name) const;

    /**
     * @brief Parses the frames array from a frame pointer array definition.
     */
    [[nodiscard]] std::vector<std::size_t>
    parse_frames_array(const std::string &content, const std::string &anim_name) const;
};

} // namespace porytiles2