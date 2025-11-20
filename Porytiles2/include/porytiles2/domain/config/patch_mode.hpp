#pragma once

namespace porytiles2 {

/**
 * @brief Specifies whether tiles can be modified during patch compilation.
 *
 * @details
 * Controls the behavior of tile handling when compiling a tileset in patch mode. In fixed mode, the compiler must use
 * only the existing tiles from the base tileset in the TilesPngWorkspace. In free mode, the compiler can add new tiles
 * to open slots in TilesPngWorkspace as needed.
 */
enum class PatchTilesMode {
    /** @brief Tiles cannot be added; must use existing tiles from base tileset */
    tiles_fixed,
    /** @brief Tiles can be freely added to open slots during patch compilation */
    tiles_free
};

/**
 * @brief Specifies whether palettes can be modified during patch compilation.
 *
 * @details
 * Controls the behavior of palette handling when compiling a tileset patch. In fixed mode, the compiler may use only
 * the existing palettes from the base tileset without modifications. In free mode, the compiler can modify unused
 * palette slots as needed.
 */
enum class PatchPalMode {
    /** @brief Palettes cannot be modified; must use existing palettes from base tileset as-is */
    pals_fixed,
    /** @brief Unused palette slots can be freely modified during patch compilation */
    pals_free
};

} // namespace porytiles2
