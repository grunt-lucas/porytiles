#pragma once

#include <cstdint>
#include <string>

namespace porytiles {

/// @brief Stores manifest metadata for Porytiles-managed tilesets.
///
/// @details
/// This class represents the contents of `tileset-manifest.json`, which is the source of truth for whether a tileset
/// is Porytiles-managed. The presence of this file in `porytiles/tilesets/{tileset_name}/` indicates the tileset is
/// managed by Porytiles.
///
/// Two cases are supported:
/// - **Imported tileset** (`imported()==true`): The tileset was imported from vanilla pokeemerald. The original field
///   values from `src/data/tilesets/headers.h` are stored, enabling the `restore-tileset` command to revert to vanilla.
/// - **Created tileset** (`imported()==false`): The tileset was created from scratch by Porytiles. There are no
/// original
///   values to restore, so `restore-tileset` will error with a clear message.
///
/// @invariant When `imported()==true`, all original field values (tiles, palettes, etc.) are non-empty
/// @invariant When `imported()==false`, original field values are empty strings and should not be used
class TilesetManifest {
  public:
    TilesetManifest() = default;

    /// @brief Constructs a TilesetManifest for an imported tileset with all original field values.
    ///
    /// @param version Schema version for future format migrations
    /// @param tiles Original `.tiles` field value (e.g., "gTilesetTiles_General")
    /// @param palettes Original `.palettes` field value (e.g., "gTilesetPalettes_General")
    /// @param metatiles Original `.metatiles` field value (e.g., "gMetatiles_General")
    /// @param metatile_attributes Original `.metatileAttributes` field value
    /// @param callback Original `.callback` field value (e.g., "InitTilesetAnim_General")
    /// @post `imported()` returns `true`
    TilesetManifest(
        std::uint32_t version,
        std::string tiles,
        std::string palettes,
        std::string metatiles,
        std::string metatile_attributes,
        std::string callback)
        : version_{version}, imported_{true}, tiles_{std::move(tiles)}, palettes_{std::move(palettes)},
          metatiles_{std::move(metatiles)}, metatile_attributes_{std::move(metatile_attributes)},
          callback_{std::move(callback)}
    {
    }

    /// @brief Creates a TilesetManifest for a tileset created from scratch by Porytiles.
    ///
    /// @details
    /// Use this factory method when creating a new tileset that doesn't originate from vanilla pokeemerald.
    /// The resulting object will have `imported()==false` and empty original field values.
    ///
    /// @param version Schema version for future format migrations
    /// @return TilesetManifest with `imported()==false`
    [[nodiscard]] static TilesetManifest for_created_tileset(std::uint32_t version)
    {
        TilesetManifest result{};
        result.version_ = version;
        result.imported_ = false;
        return result;
    }

    /// @brief Returns the schema version for format migration support.
    [[nodiscard]] std::uint32_t version() const
    {
        return version_;
    }

    /// @brief Returns whether this tileset was imported from vanilla pokeemerald.
    ///
    /// @details
    /// When `true`, the original field values are meaningful and can be used for restoration.
    /// When `false`, the tileset was created by Porytiles and cannot be restored to vanilla.
    [[nodiscard]] bool imported() const
    {
        return imported_;
    }

    /// @brief Returns the original `.tiles` field value.
    ///
    /// @pre `imported()` should be `true` for this value to be meaningful
    [[nodiscard]] const std::string &tiles() const
    {
        return tiles_;
    }

    /// @brief Returns the original `.palettes` field value.
    ///
    /// @pre `imported()` should be `true` for this value to be meaningful
    [[nodiscard]] const std::string &palettes() const
    {
        return palettes_;
    }

    /// @brief Returns the original `.metatiles` field value.
    ///
    /// @pre `imported()` should be `true` for this value to be meaningful
    [[nodiscard]] const std::string &metatiles() const
    {
        return metatiles_;
    }

    /// @brief Returns the original `.metatileAttributes` field value.
    ///
    /// @pre `imported()` should be `true` for this value to be meaningful
    [[nodiscard]] const std::string &metatile_attributes() const
    {
        return metatile_attributes_;
    }

    /// @brief Returns the original `.callback` field value.
    ///
    /// @pre `imported()` should be `true` for this value to be meaningful
    [[nodiscard]] const std::string &callback() const
    {
        return callback_;
    }

  private:
    std::uint32_t version_{};
    bool imported_{};
    std::string tiles_;
    std::string palettes_;
    std::string metatiles_;
    std::string metatile_attributes_;
    std::string callback_;
};

} // namespace porytiles
