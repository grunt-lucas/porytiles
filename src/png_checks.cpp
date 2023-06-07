#include "png_checks.h"

#include "pixel_comparators.h"
#include "cli_parser.h"
#include "tsexception.h"
#include "palette.h"
#include "tile.h"

#include <unordered_set>

namespace tscreate {
void validateMasterPngIsAPng(const std::string& masterPngPath) {
    try {
        png::image<png::rgb_pixel> masterPng(masterPngPath);
    }
    catch (const png::error& e) {
        throw TsException(masterPngPath + ": " + e.what());
    }
}

void validateMasterPngDimensions(const png::image<png::rgb_pixel>& masterPng) {
    if (masterPng.get_width() % TILE_DIMENSION != 0) {
        throw TsException("master PNG width must be divisible by 8, was: " + std::to_string(masterPng.get_width()));
    }
    if (masterPng.get_height() % TILE_DIMENSION != 0) {
        throw TsException("master PNG height must be divisible by 8, was: " + std::to_string(masterPng.get_height()));
    }
}

void validateMasterPngTilesEach16Colors(const png::image<png::rgb_pixel>& masterPng) {
    std::unordered_set<png::rgb_pixel, tscreate::rgb_pixel_hasher, tscreate::rgb_pixel_eq> uniqueRgb;
    png::uint_32 tilesWidth = masterPng.get_width() / TILE_DIMENSION;
    png::uint_32 tilesHeight = masterPng.get_height() / TILE_DIMENSION;

    for (png::uint_32 tileY = 0; tileY < tilesHeight; tileY++) {
        for (png::uint_32 tileX = 0; tileX < tilesWidth; tileX++) {
            uniqueRgb.clear();
            png::uint_32 pixelYStart = tileY * TILE_DIMENSION;
            png::uint_32 pixelXStart = tileX * TILE_DIMENSION;
            for (png::uint_32 y = 0; y < TILE_DIMENSION; y++) {
                for (png::uint_32 x = 0; x < TILE_DIMENSION; x++) {
                    uniqueRgb.insert(masterPng[pixelYStart + y][pixelXStart + x]);
                    if (uniqueRgb.size() > PAL_SIZE_4BPP) {
                        throw TsException("too many unique colors in tile: " + std::to_string(tileX) + "," +
                                          std::to_string(tileY));
                    }
                }
            }
        }
    }
}

void validateMasterPngMaxUniqueColors(const png::image<png::rgb_pixel>& masterPng) {
    std::unordered_set<png::rgb_pixel, tscreate::rgb_pixel_hasher, tscreate::rgb_pixel_eq> uniqueRgb;

    /*
     * 15 since first color of each 16 color pal is reserved for transparency, add 1 at the end for the transparency color.
     * The transparency color is shared across all palettes.
     */
    png::uint_32 maxAllowedColors = (gOptMaxPalettes * 15) + 1;

    for (png::uint_32 y = 0; y < masterPng.get_height(); y++) {
        for (png::uint_32 x = 0; x < masterPng.get_width(); x++) {
            uniqueRgb.insert(masterPng[y][x]);
            if (uniqueRgb.size() > maxAllowedColors) {
                throw TsException(
                        "too many unique colors in master PNG, max allowed: " + std::to_string(maxAllowedColors));
            }
        }
    }
}
} // namespace tscreate
