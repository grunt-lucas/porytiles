#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

#include "gsl/pointers"

#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

/**
 * @brief Appends INCBIN declarations for Porytiles-managed tileset assets.
 *
 * @details
 * This service adds new INCBIN array declarations to pokeemerald's graphics.h and metatiles.h
 * files for Porytiles-managed tilesets. The declarations follow the naming convention
 * `gTilesetTiles_PorytilesManaged_{Shorthand}` and point to deterministic paths in
 * the `porytiles_bin/` directory.
 *
 * The service uses a parse-modify-write pattern to surgically append declarations
 * without disrupting existing file content.
 *
 * Appends are idempotent:
 * any existing managed declarations for the tileset are removed before fresh ones are written.
 *
 * @see ProjectTilesetMetadataWriter for the pattern used for surgical file edits
 */
class IncbinDeclarationAppender {
  public:
    /**
     * @brief Constructs an IncbinDeclarationAppender with required dependencies.
     *
     * @param project_root Path to the pokeemerald project root directory
     * @param format Formatter for styled error messages (non-owning, must outlive this object)
     */
    IncbinDeclarationAppender(std::filesystem::path project_root, gsl::not_null<const TextFormatter *> format);

    /**
     * @brief Appends INCBIN declarations for a Porytiles-managed tileset to graphics.h.
     *
     * @details
     * Adds declarations for tiles and palettes:
     * - `gTilesetTiles_PorytilesManaged_{Shorthand}` pointing to `porytiles_bin/tiles.4bpp.lz`
     * - `gTilesetPalettes_PorytilesManaged_{Shorthand}` pointing to `porytiles_bin/palettes/x.gbapal`
     *
     * The paths are constructed using the provided bin_path_base and tileset shorthand.
     *
     * This is an idempotent operation.
     * Any existing managed declarations for the tileset are removed first,
     * then fresh ones are written after the last non-blank line,
     * which is always at preprocessor conditional depth 0.
     * This self-heals declarations previously misplaced inside a trailing preprocessor conditional,
     * such as the `#if IS_FRLG` block at the end of pokeemerald-expansion's graphics.h.
     *
     * @param tileset_name The tileset name (e.g., "gTileset_General")
     * @param bin_path_base The base path for binary assets (e.g., "data/tilesets/primary")
     * @param num_palettes Number of palette files to include (typically 6 for primary, 13 for secondary)
     * @pre tileset_name must start with "gTileset_"
     * @post Exactly one tiles and one palettes declaration for the tileset exist in graphics.h.
     * @return Success or error result with details
     */
    [[nodiscard]] ChainableResult<void> append_graphics_declarations(
        const std::string &tileset_name, const std::string &bin_path_base, std::size_t num_palettes) const;

    /**
     * @brief Appends INCBIN declarations for a Porytiles-managed tileset to metatiles.h.
     *
     * @details
     * Adds declarations for metatiles and attributes:
     * - `gMetatiles_PorytilesManaged_{Shorthand}` pointing to `porytiles_bin/metatiles.bin`
     * - `gMetatileAttributes_PorytilesManaged_{Shorthand}` pointing to `porytiles_bin/metatile_attributes.bin`
     *
     * The attribute declaration uses `const u16` / `INCBIN_U16` when @p metatile_attr_size is 2,
     * or `const u32` / `INCBIN_U32` when @p metatile_attr_size is 4.
     *
     * This is an idempotent upsert.
     * Any existing managed declarations for the tileset are removed first,
     * then fresh ones are written after the last non-blank line,
     * which is always at preprocessor conditional depth 0.
     * This self-heals declarations previously misplaced inside a trailing preprocessor conditional,
     * such as the `#if !IS_FRLG ... #else ... #endif` block at the end of pokeemerald-expansion's metatiles.h.
     *
     * @param tileset_name The tileset name (e.g., "gTileset_General")
     * @param bin_path_base The base path for binary assets (e.g., "data/tilesets/primary")
     * @param metatile_attr_size The size in bytes of each metatile attribute entry (2 or 4)
     * @pre tileset_name must start with "gTileset_"
     * @pre @p metatile_attr_size must be 2 or 4
     * @post Exactly one metatiles and one attributes declaration for the tileset exist in metatiles.h.
     * @return Success or error result with details
     */
    [[nodiscard]] ChainableResult<void> append_metatiles_declarations(
        const std::string &tileset_name, const std::string &bin_path_base, std::size_t metatile_attr_size) const;

    /**
     * @brief Removes INCBIN declarations for a Porytiles-managed tileset (for restore workflow).
     *
     * @details
     * Removes all declarations matching `*_PorytilesManaged_{Shorthand}` pattern from
     * both graphics.h and metatiles.h.
     *
     * @param tileset_name The tileset name (e.g., "gTileset_General")
     * @pre tileset_name must start with "gTileset_"
     * @return Success or error result with details
     */
    [[nodiscard]] ChainableResult<void> remove_declarations(const std::string &tileset_name) const;

  private:
    std::filesystem::path project_root_;
    const TextFormatter *format_;
};

} // namespace porytiles
