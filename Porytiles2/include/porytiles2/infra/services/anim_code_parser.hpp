#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "gsl/pointers"

#include "porytiles2/domain/models/animation_params.hpp"
#include "porytiles2/utilities/dynamic_cased_name.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief Parses C code to extract tileset animation parameters using callback chain discovery.
 *
 * @details
 * AnimCodeParser extracts AnimationParams from C source files by following the callback chain rather than relying on
 * hardcoded function prefixes. The discovery process works as follows:
 *
 * 1. **Callback function** (e.g., `InitTilesetAnim_General`): Contains assignment to the driver callback pointer
 * 2. **Driver function** (e.g., `TilesetAnim_General`): Contains timer conditions and queue function calls
 * 3. **Queue functions**: Contain `AppendTilesetAnimToBuffer` calls with animation parameters
 *
 * The only required invariant is that animation frame arrays follow the naming convention:
 * `gTilesetAnims_{TilesetName}_{AnimName}{_OptionalSuffix}`
 *
 * The parser extracts:
 * - tile_offset from TILE_OFFSET_4BPP(X) in AppendTilesetAnimToBuffer calls
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
     * @param diag UserDiagnostics for user diagnostics
     */
    explicit AnimCodeParser(gsl::not_null<const TextFormatter *> format, gsl::not_null<const UserDiagnostics *> diag)
        : format_{format}, diag_{diag}
    {
    }

    /**
     * @brief Parses animation parameters by following the callback chain.
     *
     * @details
     * This is the primary parsing method that discovers animations automatically without requiring the caller to
     * provide expected animation names. It follows the callback chain:
     *
     * 1. Parses the callback function (e.g., `InitTilesetAnim_General`) to find the driver function assignment
     * 2. Parses the driver function to find timer conditions and queue function calls
     * 3. For each queue function, finds `AppendTilesetAnimToBuffer` calls and extracts:
     *    - Animation name from the first argument (e.g., `gTilesetAnims_General_Flower[i]` → "flower")
     *    - tile_offset from the second argument
     *    - tile_count from the third argument
     * 4. Parses frame pointer arrays to get frame sequences
     *
     * This approach removes the brittleness of requiring hardcoded function prefixes like `TilesetAnim_` or
     * `QueueAnimTiles_`. Driver and queue functions can have any name as long as they follow the calling pattern.
     *
     * @param c_file_path Path to the C file containing animation code (tileset_anims.c or generated_anim_code.h)
     * @param callback_func_name The callback function name from the tileset struct (e.g., "InitTilesetAnim_General")
     * @param pascal_case_tileset The tileset name without prefix (e.g., "General" from "gTileset_General")
     * @param porytiles_managed True if this is a Porytiles-managed tileset (uses "PorytilesManaged_" prefix)
     * @pre The pascal_case_tileset param must be in valid PascalCase
     * @return Map of animation names (snake_case) to their parsed parameters, or error
     */
    [[nodiscard]] ChainableResult<std::map<DynamicCasedName, AnimationParams>> parse_from_callback(
        const std::filesystem::path &c_file_path,
        const std::string &callback_func_name,
        const std::string &pascal_case_tileset,
        bool porytiles_managed) const;

  private:
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles2
