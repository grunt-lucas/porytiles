#pragma once

#include <filesystem>
#include <vector>

namespace porytiles {

/**
 * @brief Represents resolved filesystem paths for tileset artifacts from INCBIN declarations.
 *
 * @details
 * TilesetArtifactPaths holds the actual file paths extracted from INCBIN macro declarations in pokeemerald's graphics.h
 * and metatiles.h files (and a few other assorted files):
 * @code
 * // From graphics.h:
 * const u32 gTilesetTiles_General[] = INCBIN_U32("data/tilesets/primary/general/tiles.4bpp");
 * const u16 gTilesetPalettes_General[][16] = {
 *     INCBIN_U16("data/tilesets/primary/general/palettes/00.gbapal"),
 *     ...
 * };
 *
 * // From metatiles.h:
 * const u16 gMetatiles_General[] = INCBIN_U16("data/tilesets/primary/general/metatiles.bin");
 * const u16 gMetatileAttributes_General[] = INCBIN_U16("data/tilesets/primary/general/metatile_attributes.bin");
 * @endcode
 *
 * This domain model provides direct access to individual artifact paths.
 *
 * @invariant tiles_path_ is never empty
 * @invariant palette_paths_ contains at least one path
 * @invariant metatiles_path_ is never empty
 * @invariant metatile_attributes_path_ is never empty
 */
class ProjectTilesetArtifactPaths {
  public:
    /**
     * @brief Constructs TilesetArtifactPaths from resolved INCBIN paths.
     *
     * @param tiles_path Path to tiles file (e.g., "data/tilesets/primary/general/tiles.4bpp")
     * @param palette_paths Paths to palette files (e.g., [.../palettes/00.gbapal, ...])
     * @param metatiles_path Path to metatiles file (e.g., "data/tilesets/primary/general/metatiles.bin")
     * @param metatile_attributes_path Path to attributes file (e.g., ".../metatile_attributes.bin")
     */
    ProjectTilesetArtifactPaths(
        std::filesystem::path tiles_path,
        std::vector<std::filesystem::path> palette_paths,
        std::filesystem::path metatiles_path,
        std::filesystem::path metatile_attributes_path)
        : tiles_path_{std::move(tiles_path)}, palette_paths_{std::move(palette_paths)},
          metatiles_path_{std::move(metatiles_path)}, metatile_attributes_path_{std::move(metatile_attributes_path)}
    {
    }

    /**
     * @brief Returns the path to the tiles file.
     *
     * @return The tiles path (e.g., "data/tilesets/primary/general/tiles.4bpp")
     */
    [[nodiscard]] const std::filesystem::path &tiles_path() const
    {
        return tiles_path_;
    }

    /**
     * @brief Returns all palette file paths.
     *
     * @return Vector of palette paths (typically 16 entries)
     */
    [[nodiscard]] const std::vector<std::filesystem::path> &palette_paths() const
    {
        return palette_paths_;
    }

    /**
     * @brief Returns the path to the metatiles file.
     *
     * @return The metatiles path (e.g., "data/tilesets/primary/general/metatiles.bin")
     */
    [[nodiscard]] const std::filesystem::path &metatiles_path() const
    {
        return metatiles_path_;
    }

    /**
     * @brief Returns the path to the metatile attributes file.
     *
     * @return The attributes path (e.g., "data/tilesets/primary/general/metatile_attributes.bin")
     */
    [[nodiscard]] const std::filesystem::path &metatile_attributes_path() const
    {
        return metatile_attributes_path_;
    }

  private:
    std::filesystem::path tiles_path_;
    std::vector<std::filesystem::path> palette_paths_;
    std::filesystem::path metatiles_path_;
    std::filesystem::path metatile_attributes_path_;
};

} // namespace porytiles
