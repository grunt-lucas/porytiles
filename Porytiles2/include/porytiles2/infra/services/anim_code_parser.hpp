#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "gsl/pointers"

#include "porytiles2/domain/models/animation_params.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief Parses C code to extract tileset animation parameters.
 *
 * @details
 * AnimCodeParser extracts AnimationParams from C source files using proper C token-based parsing via CParserFacade. It
 * supports two parsing modes:
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
 * Parsing errors produce rich diagnostics with source file context via FileHighlightPrinter.
 *
 * Note: Animation frame PNG files are loaded separately by the artifact reader; this parser only extracts the
 * configuration parameters embedded in C code.
 */
class AnimCodeParser {
  public:
    /**
     * @brief Constructs an AnimCodeParser with a text formatter for error messages.
     *
     * @param format Formatter for error message styling (non-owning, must outlive parser)
     */
    explicit AnimCodeParser(gsl::not_null<const TextFormatter *> format);
    /**
     * @brief Parses a Porytiles-generated animation header file.
     *
     * @details
     * Parses `generated_anim_code.h` files created by AnimCodeGenerator. Extracts all animation parameters including
     * tile_offset and tile_count which are embedded in the QueueAnimTiles functions.
     *
     * @param header_path Path to the generated_anim_code.h file
     * @param tileset_name The name of the tileset to extract animations for
     * @return Map of animation names to their parsed parameters, or error
     */
    [[nodiscard]] ChainableResult<std::map<std::string, AnimationParams>>
    parse_generated_header(const std::filesystem::path &header_path, const std::string &tileset_name) const;

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
    const TextFormatter *format_;
};

} // namespace porytiles2