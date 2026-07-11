#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "gsl/pointers"

#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

/// @brief Provides surgical update capability for tileset metadata in headers.h files.
///
/// @details
/// This class enables targeted modifications to specific fields within tileset struct declarations in pokeemerald
/// headers.h files. Unlike a full rewrite approach, it performs surgical line-level edits to preserve formatting,
/// comments, and other content in the file.
///
/// Supports updating any field in the Tileset struct, including .tiles, .palettes, .metatiles, .metatileAttributes,
/// and .callback.
class ProjectTilesetMetadataWriter {
  public:
    ProjectTilesetMetadataWriter(std::filesystem::path project_root, gsl::not_null<const TextFormatter *> format);

    /// @brief Updates the .callback field for a specific tileset.
    ///
    /// @details
    /// Locates the tileset struct by name in headers.h and surgically replaces the .callback field value. The new value
    /// is written exactly as provided - callers should pass either "NULL" or "InitTilesetAnim_<name>" format.
    ///
    /// @param tileset_name The tileset name (e.g., "gTileset_General") - must match exactly
    /// @param new_callback_value The exact value to write (e.g., "InitTilesetAnim_General" or "NULL")
    /// @pre tileset_name must correspond to an existing tileset in headers.h
    /// @pre new_callback_value must be a valid C identifier or "NULL"
    /// @return Success or error result with details
    [[nodiscard]] ChainableResult<void>
    update_callback(const std::string &tileset_name, const std::string &new_callback_value) const;

    /// @brief Updates multiple fields for a specific tileset in headers.h.
    ///
    /// @details
    /// Locates the tileset struct by name and surgically replaces specified field values. All updates are performed in
    /// a single parse-modify-write cycle for efficiency. Field names should be provided WITHOUT the leading dot (e.g.,
    /// "tiles" not ".tiles").
    ///
    /// @param tileset_name The tileset name (e.g., "gTileset_General") - must match exactly
    /// @param field_updates Map of field names to new values
    /// @pre tileset_name must correspond to an existing tileset in headers.h
    /// @pre All field names in field_updates must exist in the tileset struct
    /// @return Success or error result with details
    [[nodiscard]] ChainableResult<void>
    update_fields(const std::string &tileset_name, const std::map<std::string, std::string> &field_updates) const;

    /// @brief Updates asset fields to Porytiles-managed values.
    ///
    /// @details
    /// Generates new field values using the "PorytilesManaged" naming convention:
    /// - .tiles = gTilesetTiles_PorytilesManaged_{Shorthand}
    /// - .palettes = gTilesetPalettes_PorytilesManaged_{Shorthand}
    /// - .metatiles = gMetatiles_PorytilesManaged_{Shorthand}
    /// - .metatileAttributes = gMetatileAttributes_PorytilesManaged_{Shorthand}
    ///
    /// @param tileset_name The tileset name (e.g., "gTileset_General")
    /// @pre tileset_name must start with "gTileset_"
    /// @pre tileset_name must correspond to an existing tileset in headers.h
    /// @return Success or error result with details
    [[nodiscard]] ChainableResult<void> update_to_porytiles_managed(const std::string &tileset_name) const;

    /// @brief Creates a new tileset struct and appends it to headers.h.
    ///
    /// @details
    /// Generates a complete Tileset struct with Porytiles-managed field values and appends it to the end of headers.h.
    /// The struct uses the standard pokeemerald format:
    ///
    /// @code
    /// const struct Tileset gTileset_MyTileset =
    /// {
    ///     .isCompressed = TRUE,
    ///     .isSecondary = FALSE,
    ///     .tiles = gTilesetTiles_PorytilesManaged_MyTileset,
    ///     .palettes = gTilesetPalettes_PorytilesManaged_MyTileset,
    ///     .metatiles = gMetatiles_PorytilesManaged_MyTileset,
    ///     .metatileAttributes = gMetatileAttributes_PorytilesManaged_MyTileset,
    ///     .callback = NULL,
    /// };
    /// @endcode
    ///
    /// @param tileset_name The tileset name (e.g., "gTileset_MyTileset")
    /// @param is_secondary If true, sets .isSecondary = TRUE; if false, sets .isSecondary = FALSE
    /// @pre tileset_name must start with "gTileset_"
    /// @pre tileset_name must not already exist in headers.h
    /// @return Success or error result with details
    /// @post A new tileset struct exists at the end of headers.h
    [[nodiscard]] ChainableResult<void> create_tileset_struct(const std::string &tileset_name, bool is_secondary) const;

  private:
    std::filesystem::path project_root_;
    const TextFormatter *format_;
};

} // namespace porytiles
