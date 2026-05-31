#pragma once

#include <optional>
#include <string>

namespace porytiles {

/**
 * @brief Represents raw parsed metadata from a tileset struct in headers.h.
 *
 * @details
 * ProjectTilesetMetadata captures the key fields from a pokeemerald tileset struct definition:
 * @code
 * const struct Tileset gTileset_General = {
 *     .isCompressed = TRUE,
 *     .isSecondary = FALSE,
 *     .tiles = gTilesetTiles_General,
 *     .palettes = gTilesetPalettes_General,
 *     .metatiles = gMetatiles_General,
 *     .metatileAttributes = gMetatileAttributes_General,
 *     .callback = InitTilesetAnim_General,
 * };
 * @endcode
 *
 * This model contains only the raw field values from the struct:
 * - Whether a tileset is primary or secondary (via is_secondary())
 * - The variable names referenced for tiles, palettes, metatiles, etc.
 * - Whether the tileset has animations (via has_animations())
 * - The callback function name if present
 *
 * The variable names can then be used by ProjectTilesetArtifactKeyProvider to look up INCBIN
 * declarations and resolve actual file paths.
 *
 * @invariant name_ is always a valid TilesetName
 * @invariant tiles_var_, palettes_var_, metatiles_var_, metatile_attributes_var_ are never empty
 */
class ProjectTilesetMetadata {
  public:
    /**
     * @brief Constructs a TilesetMetadata from parsed struct fields.
     *
     * @param name The tileset name (e.g., gTileset_General)
     * @param is_secondary True if this is a secondary tileset, false for primary
     * @param tiles_var The variable name for tiles (e.g., gTilesetTiles_General)
     * @param palettes_var The variable name for palettes (e.g., gTilesetPalettes_General)
     * @param metatiles_var The variable name for metatiles (e.g., gMetatiles_General)
     * @param metatile_attributes_var The variable name for attributes (e.g., gMetatileAttributes_General)
     * @param callback_func The callback function name if animations exist, or nullopt if none
     */
    ProjectTilesetMetadata(
        std::string name,
        bool is_secondary,
        std::string tiles_var,
        std::string palettes_var,
        std::string metatiles_var,
        std::string metatile_attributes_var,
        std::optional<std::string> callback_func)
        : name_{std::move(name)}, is_secondary_{is_secondary}, tiles_var_{std::move(tiles_var)},
          palettes_var_{std::move(palettes_var)}, metatiles_var_{std::move(metatiles_var)},
          metatile_attributes_var_{std::move(metatile_attributes_var)}, callback_func_{std::move(callback_func)}
    {
    }

    /**
     * @brief Returns the tileset name.
     *
     * @return A const reference to the TilesetName
     */
    [[nodiscard]] const std::string &name() const
    {
        return name_;
    }

    /**
     * @brief Returns whether this is a secondary tileset.
     *
     * @return True for secondary tilesets, false for primary
     */
    [[nodiscard]] bool is_secondary() const
    {
        return is_secondary_;
    }

    /**
     * @brief Returns the tiles variable name.
     *
     * @return The variable name (e.g., "gTilesetTiles_General")
     */
    [[nodiscard]] const std::string &tiles_var() const
    {
        return tiles_var_;
    }

    /**
     * @brief Returns the palettes variable name.
     *
     * @return The variable name (e.g., "gTilesetPalettes_General")
     */
    [[nodiscard]] const std::string &palettes_var() const
    {
        return palettes_var_;
    }

    /**
     * @brief Returns the metatiles variable name.
     *
     * @return The variable name (e.g., "gMetatiles_General")
     */
    [[nodiscard]] const std::string &metatiles_var() const
    {
        return metatiles_var_;
    }

    /**
     * @brief Returns the metatile attributes variable name.
     *
     * @return The variable name (e.g., "gMetatileAttributes_General")
     */
    [[nodiscard]] const std::string &metatile_attributes_var() const
    {
        return metatile_attributes_var_;
    }

    /**
     * @brief Returns the animation callback function name, if present.
     *
     * @return The function name (e.g., "InitTilesetAnim_General") or nullopt if no animations
     */
    [[nodiscard]] const std::optional<std::string> &callback_func() const
    {
        return callback_func_;
    }

    /**
     * @brief Checks if this tileset has animations.
     *
     * @details
     * A tileset has animations if its callback field is set to a non-NULL function name.
     *
     * @return True if callback_func is present and not "NULL"
     */
    [[nodiscard]] bool has_animations() const
    {
        return callback_func_.has_value() && callback_func_.value() != "NULL";
    }

  private:
    std::string name_;
    bool is_secondary_;
    std::string tiles_var_;
    std::string palettes_var_;
    std::string metatiles_var_;
    std::string metatile_attributes_var_;
    std::optional<std::string> callback_func_;
};

} // namespace porytiles
