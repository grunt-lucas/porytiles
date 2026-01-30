#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace porytiles2 {

/**
 * @brief Maps animation names to their ordered frame file paths.
 *
 * @details
 * AnimationFramePaths stores the resolved filesystem paths for animation frame files
 * discovered from INCBIN declarations in tileset_anims.c or generated_anim_code.h.
 *
 * Key: Animation name in snake_case (e.g., "flower", "water", "sand_water_edge")
 * Value: Ordered vector of frame paths where index corresponds to frame number
 *
 * Example structure:
 * @code
 * {
 *   "flower": ["data/tilesets/.../anim/flower/0.4bpp", "data/tilesets/.../anim/flower/1.4bpp"],
 *   "water": ["data/tilesets/.../anim/water/0.4bpp", ..., "data/tilesets/.../anim/water/7.4bpp"]
 * }
 * @endcode
 */
using AnimationFramePaths = std::map<std::string, std::vector<std::filesystem::path>>;

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
     * @param animation_frame_paths Animation frame paths grouped by animation name (optional, may be empty)
     */
    ProjectTilesetArtifactPaths(
        std::filesystem::path tiles_path,
        std::vector<std::filesystem::path> palette_paths,
        std::filesystem::path metatiles_path,
        std::filesystem::path metatile_attributes_path,
        AnimationFramePaths animation_frame_paths = {})
        : tiles_path_{std::move(tiles_path)}, palette_paths_{std::move(palette_paths)},
          metatiles_path_{std::move(metatiles_path)}, metatile_attributes_path_{std::move(metatile_attributes_path)},
          animation_frame_paths_{std::move(animation_frame_paths)}
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

    /**
     * @brief Returns animation frame paths grouped by animation name.
     *
     * @details
     * Returns the discovered animation frame paths from INCBIN declarations. May be empty if the tileset has no
     * animations or if animation discovery was not performed.
     *
     * @return Map of animation names to ordered frame paths
     */
    [[nodiscard]] const AnimationFramePaths &animation_frame_paths() const
    {
        return animation_frame_paths_;
    }

    /**
     * @brief Checks if animation frame paths have been discovered.
     *
     * @return true if animation_frame_paths_ is not empty
     */
    [[nodiscard]] bool has_animation_frame_paths() const
    {
        return !animation_frame_paths_.empty();
    }

  private:
    std::filesystem::path tiles_path_;
    std::vector<std::filesystem::path> palette_paths_;
    std::filesystem::path metatiles_path_;
    std::filesystem::path metatile_attributes_path_;
    AnimationFramePaths animation_frame_paths_;
};

} // namespace porytiles2
