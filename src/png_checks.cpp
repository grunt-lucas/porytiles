#include "png_checks.h"

#include "cli_parser.h"
#include "tsexception.h"
#include "palette.h"
#include "tile.h"
#include "rgb_tiled_png.h"
#include "rgb_color.h"

#include <doctest.h>
#include <png.hpp>
#include <unordered_set>
#include <filesystem>

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
        throw TsException("master PNG height must be divisible by 8, was: " + std::to_string(masterPng.get_height()));
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

void validateMasterPngMaxUniqueColors(const RgbTiledPng& png, size_t maxPalettes) {
    std::unordered_set<RgbColor> uniqueRgb;

    /*
     * 15 since first color of each 16 color pal is reserved for transparency. We are ignoring the transparency color in
     * our processing here.
     */
    size_t maxAllowedColors = (maxPalettes * 15);

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

/*
 * Test Cases
 */
TEST_CASE("It should throw for invalid PNG file") {
    REQUIRE(std::filesystem::exists("res/tests/i_am_not_a_png.txt"));

    CHECK_THROWS_WITH_AS(porytiles::validateMasterPngIsAPng("res/tests/i_am_not_a_png.txt"),
                         "res/tests/i_am_not_a_png.txt: Not a PNG file",
                         const porytiles::TsException&);
}

TEST_CASE("It should throw for invalid dimensions") {
    REQUIRE(std::filesystem::exists("res/tests/invalid_width.png"));
    REQUIRE(std::filesystem::exists("res/tests/invalid_height.png"));

    SUBCASE("It should throw for invalid width") {
        png::image<png::rgb_pixel> invalidWidth{"res/tests/invalid_width.png"};
        std::string expectedErrorMessage = "master PNG width must be divisible by 8, was: " +
                                           std::to_string(invalidWidth.get_width());
        CHECK_THROWS_WITH_AS(porytiles::validateMasterPngDimensions(invalidWidth),
                             expectedErrorMessage.c_str(), const porytiles::TsException&);
    }
    SUBCASE("It should throw for invalid height") {
        png::image<png::rgb_pixel> invalidHeight{"res/tests/invalid_height.png"};
        std::string expectedErrorMessage = "master PNG height must be divisible by 8, was: " +
                                           std::to_string(invalidHeight.get_height());
        CHECK_THROWS_WITH_AS(porytiles::validateMasterPngDimensions(invalidHeight),
                             expectedErrorMessage.c_str(), const porytiles::TsException&);
    }
}

TEST_CASE("It should pass, tile has 15 colors + transparency") {
    REQUIRE(std::filesystem::exists("res/tests/15_colors_in_tile_plus_transparency.png"));

    png::image<png::rgb_pixel> png{"res/tests/15_colors_in_tile_plus_transparency.png"};
    porytiles::RgbTiledPng tiledPng{png};
    porytiles::validateMasterPngTilesEach16Colors(tiledPng);
}

TEST_CASE("It should throw for too many unique colors in one tile") {
    REQUIRE(std::filesystem::exists("res/tests/too_many_unique_colors_in_tile.png"));

    png::image<png::rgb_pixel> png{"res/tests/too_many_unique_colors_in_tile.png"};
    porytiles::RgbTiledPng tiledPng{png};
    CHECK_THROWS_WITH_AS(porytiles::validateMasterPngTilesEach16Colors(tiledPng),
                         "too many unique colors in tile 0 (row 0, col 0)", const porytiles::TsException&);
}

TEST_CASE("It should throw for too many unique colors in entire image") {
    REQUIRE(std::filesystem::exists("res/tests/too_many_unique_colors_in_image.png"));

    png::image<png::rgb_pixel> png{"res/tests/too_many_unique_colors_in_image.png"};
    porytiles::RgbTiledPng tiledPng{png};
    std::string expectedErrorMessage = "too many unique colors in master PNG, max allowed: 15";
    CHECK_THROWS_WITH_AS(porytiles::validateMasterPngMaxUniqueColors(tiledPng, 1), expectedErrorMessage.c_str(),
                         const porytiles::TsException&);
}
