#include "png_checks.h"

#include "cli_parser.h"
#include "tsexception.h"
#include "palette.h"
#include "tile.h"
#include "rgb_tiled_png.h"
#include "rgb_color.h"

#include <unordered_set>

namespace porytiles {
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
        throw TsException("master PNG rows must be divisible by 8, was: " + std::to_string(masterPng.get_height()));
    }
}

void validateMasterPngTilesEach16Colors(const RgbTiledPng& png) {
    std::unordered_set<RgbColor> uniqueRgb;
    size_t index = 0;

    for (size_t row = 0; row < png.getHeight(); row++) {
        for (size_t col = 0; col < png.getWidth(); col++) {
            uniqueRgb.clear();
            const RgbTile& tile = png.tileAt(row, col);
            for (size_t pixelRow = 0; pixelRow < TILE_DIMENSION; pixelRow++) {
                for (size_t pixelCol = 0; pixelCol < TILE_DIMENSION; pixelCol++) {
                    if (tile.getPixel(pixelRow, pixelCol) != gOptTransparentColor) {
                        uniqueRgb.insert(tile.getPixel(pixelRow, pixelCol));
                    }
                    if (uniqueRgb.size() > PAL_SIZE_4BPP) {
                        throw TsException(
                                "too many unique colors in tile " + std::to_string(index) +
                                " (row " + std::to_string(row) + ", col " + std::to_string(col) + ")"
                        );
                    }
                }
            }
            index++;
        }
    }
}

void validateMasterPngMaxUniqueColors(const RgbTiledPng& png) {
    std::unordered_set<RgbColor> uniqueRgb;

    /*
     * 15 since first color of each 16 color pal is reserved for transparency. We are ignoring the transparency color in
     * our processing here.
     */
    size_t maxAllowedColors = (gOptMaxPalettes * 15);

    for (size_t index = 0; index < png.size(); index++) {
        const RgbTile& tile = png.tileAt(index);
        for (size_t pixelRow = 0; pixelRow < TILE_DIMENSION; pixelRow++) {
            for (size_t pixelCol = 0; pixelCol < TILE_DIMENSION; pixelCol++) {
                if (tile.getPixel(pixelRow, pixelCol) != gOptTransparentColor) {
                    uniqueRgb.insert(tile.getPixel(pixelRow, pixelCol));
                }
                if (uniqueRgb.size() > maxAllowedColors) {
                    throw TsException(
                            "too many unique colors in master PNG, max allowed: " +
                            std::to_string(maxAllowedColors));
                }
            }
        }
    }
}
} // namespace porytiles
