#include "compiler.h"

#include <png.hpp>

#include "doctest.h"
#include "config.h"
#include "ptexception.h"
#include "constants.h"
#include "types.h"
#include "png_frontend.h"

namespace porytiles {

// TODO : change this to receive CompilerContext once I have made that type available
static int insertRGBA(const Config& config, NormalizedPalette& palette, RGBA32 rgba) {
    if (rgba.alpha == ALPHA_TRANSPARENT || rgba == config.transparencyColor) {
        return 0;
    }
    else if (rgba.alpha == ALPHA_OPAQUE) {
        /*
         * TODO : we lose color precision here, it would be nice to warn the user if two distinct RGBA colors they used
         * in the master sheet are going to collapse to one BGR color on the GBA. This should default fail the build,
         * but a compiler flag '--allow-color-precision-loss' would disable this warning
         */
        auto bgr = rgbaToBgr(rgba);
        auto itrAtBgr = std::find(std::begin(palette.colors) + 1, std::begin(palette.colors) + palette.size, bgr);
        auto bgrPosInPalette = itrAtBgr - std::begin(palette.colors);
        if (bgrPosInPalette == palette.size) {
            // palette size will grow as we add to it
            if (palette.size == PAL_SIZE) {
                // TODO : better error context
                throw PtException{"too many colors"};
            }
            palette.colors[palette.size++] = bgr;
        }
        return bgrPosInPalette;
    }
    throw PtException{"invalid alpha value: " + std::to_string(rgba.alpha)};
}

static NormalizedTile candidate(const Config& config, const RGBATile& rgba, bool hFlip, bool vFlip) {
    /*
     * NOTE: This only produces a _candidate_ normalized tile (a different choice of hFlip/vFlip might be the normal
     * form). We'll use this to generate candidates to find the true normal form.
     */
    NormalizedTile candidateTile;
    // Size is 1 to account for eventual transparent color in first palette slot
    candidateTile.palette.size = 1;
    // TODO : same color precision note as above in insertRGBA
    candidateTile.palette.colors[0] = rgbaToBgr(config.transparencyColor);
    candidateTile.hFlip = hFlip;
    candidateTile.vFlip = vFlip;

    for (std::size_t row = 0; row < TILE_SIDE_LENGTH; row++) {
        for (std::size_t col = 0; col < TILE_SIDE_LENGTH; col++) {
            std::size_t rowWithFlip = vFlip ? TILE_SIDE_LENGTH - 1 - row : row;
            std::size_t colWithFlip = hFlip ? TILE_SIDE_LENGTH - 1 - col : col;
            candidateTile.setPixel(row, col, insertRGBA(config, candidateTile.palette, rgba.getPixel(rowWithFlip, colWithFlip)));
        }
    }

    return candidateTile;
}

}

TEST_CASE("insertRGBA should add new colors in order and return the correct index for a given color") {
    porytiles::Config config;
    config.transparencyColor = porytiles::RGBA_MAGENTA;

    porytiles::NormalizedPalette palette1;
    palette1.size = 1;
    palette1.colors = {};

    // invalid alpha value, must be opaque or transparent
    CHECK_THROWS_WITH_AS(insertRGBA(config, palette1, porytiles::RGBA32{0, 0, 0, 12}),
                            "invalid alpha value: 12",
                            const porytiles::PtException&);

    // Transparent should return 0
    CHECK(insertRGBA(config, palette1, porytiles::RGBA_MAGENTA) == 0);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{0, 0, 0, porytiles::ALPHA_TRANSPARENT}) == 0);

    // insert colors
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{0, 0, 0, porytiles::ALPHA_OPAQUE}) == 1);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{8, 0, 0, porytiles::ALPHA_OPAQUE}) == 2);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{16, 0, 0, porytiles::ALPHA_OPAQUE}) == 3);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{24, 0, 0, porytiles::ALPHA_OPAQUE}) == 4);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{32, 0, 0, porytiles::ALPHA_OPAQUE}) == 5);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{40, 0, 0, porytiles::ALPHA_OPAQUE}) == 6);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{48, 0, 0, porytiles::ALPHA_OPAQUE}) == 7);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{56, 0, 0, porytiles::ALPHA_OPAQUE}) == 8);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{64, 0, 0, porytiles::ALPHA_OPAQUE}) == 9);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{72, 0, 0, porytiles::ALPHA_OPAQUE}) == 10);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{80, 0, 0, porytiles::ALPHA_OPAQUE}) == 11);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{88, 0, 0, porytiles::ALPHA_OPAQUE}) == 12);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{96, 0, 0, porytiles::ALPHA_OPAQUE}) == 13);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{104, 0, 0, porytiles::ALPHA_OPAQUE}) == 14);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{112, 0, 0, porytiles::ALPHA_OPAQUE}) == 15);

    // repeat colors should return their indexes
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{72, 0, 0, porytiles::ALPHA_OPAQUE}) == 10);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{112, 0, 0, porytiles::ALPHA_OPAQUE}) == 15);

    // Transparent should still return 0
    CHECK(insertRGBA(config, palette1, porytiles::RGBA_MAGENTA) == 0);
    CHECK(insertRGBA(config, palette1, porytiles::RGBA32{0, 0, 0, porytiles::ALPHA_TRANSPARENT}) == 0);

    // Should throw, palette full
    CHECK_THROWS_WITH_AS(insertRGBA(config, palette1, porytiles::RGBA_CYAN),
                            "too many colors",
                            const porytiles::PtException&);
}

TEST_CASE("candidate should return the NormalizedTile with requested flips") {
    porytiles::Config config;
    config.transparencyColor = porytiles::RGBA_MAGENTA;

    REQUIRE(std::filesystem::exists("res/tests/corners.png"));
    png::image<png::rgba_pixel> png1{"res/tests/corners.png"};
    porytiles::DecompiledTileset tiles = porytiles::importTilesFrom(png1);
    porytiles::RGBATile tile = tiles.tiles[0];

    SUBCASE("case: no flips") {
        porytiles::NormalizedTile candidate = porytiles::candidate(config, tile, false, false);
        CHECK(candidate.palette.size == 9);
        CHECK(candidate.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
        CHECK(candidate.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
        CHECK(candidate.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_YELLOW));
        CHECK(candidate.palette.colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
        CHECK(candidate.palette.colors[4] == porytiles::rgbaToBgr(porytiles::RGBA_WHITE));
        CHECK(candidate.palette.colors[5] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
        CHECK(candidate.palette.colors[6] == porytiles::rgbaToBgr(porytiles::RGBA_BLACK));
        CHECK(candidate.palette.colors[7] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
        CHECK(candidate.palette.colors[8] == porytiles::rgbaToBgr(porytiles::RGBA_GREY));
        CHECK(candidate.pixels.paletteIndexes[0] == 1);
        CHECK(candidate.pixels.paletteIndexes[7] == 2);
        CHECK(candidate.pixels.paletteIndexes[9] == 3);
        CHECK(candidate.pixels.paletteIndexes[14] == 4);
        CHECK(candidate.pixels.paletteIndexes[18] == 2);
        CHECK(candidate.pixels.paletteIndexes[21] == 5);
        CHECK(candidate.pixels.paletteIndexes[42] == 3);
        CHECK(candidate.pixels.paletteIndexes[45] == 1);
        CHECK(candidate.pixels.paletteIndexes[49] == 6);
        CHECK(candidate.pixels.paletteIndexes[54] == 7);
        CHECK(candidate.pixels.paletteIndexes[56] == 8);
        CHECK(candidate.pixels.paletteIndexes[63] == 5);
    }

    SUBCASE("case: hFlip") {
        porytiles::NormalizedTile candidate = porytiles::candidate(config, tile, true, false);
        CHECK(candidate.palette.size == 9);
        CHECK(candidate.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
        CHECK(candidate.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_YELLOW));
        CHECK(candidate.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
        CHECK(candidate.palette.colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_WHITE));
        CHECK(candidate.palette.colors[4] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
        CHECK(candidate.palette.colors[5] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
        CHECK(candidate.palette.colors[6] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
        CHECK(candidate.palette.colors[7] == porytiles::rgbaToBgr(porytiles::RGBA_BLACK));
        CHECK(candidate.palette.colors[8] == porytiles::rgbaToBgr(porytiles::RGBA_GREY));
        CHECK(candidate.pixels.paletteIndexes[0] == 1);
        CHECK(candidate.pixels.paletteIndexes[7] == 2);
        CHECK(candidate.pixels.paletteIndexes[9] == 3);
        CHECK(candidate.pixels.paletteIndexes[14] == 4);
        CHECK(candidate.pixels.paletteIndexes[18] == 5);
        CHECK(candidate.pixels.paletteIndexes[21] == 1);
        CHECK(candidate.pixels.paletteIndexes[42] == 2);
        CHECK(candidate.pixels.paletteIndexes[45] == 4);
        CHECK(candidate.pixels.paletteIndexes[49] == 6);
        CHECK(candidate.pixels.paletteIndexes[54] == 7);
        CHECK(candidate.pixels.paletteIndexes[56] == 5);
        CHECK(candidate.pixels.paletteIndexes[63] == 8);
    }

    SUBCASE("case: vFlip") {
        porytiles::NormalizedTile candidate = porytiles::candidate(config, tile, false, true);
        CHECK(candidate.palette.size == 9);
        CHECK(candidate.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
        CHECK(candidate.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_GREY));
        CHECK(candidate.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
        CHECK(candidate.palette.colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_BLACK));
        CHECK(candidate.palette.colors[4] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
        CHECK(candidate.palette.colors[5] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
        CHECK(candidate.palette.colors[6] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
        CHECK(candidate.palette.colors[7] == porytiles::rgbaToBgr(porytiles::RGBA_YELLOW));
        CHECK(candidate.palette.colors[8] == porytiles::rgbaToBgr(porytiles::RGBA_WHITE));
        CHECK(candidate.pixels.paletteIndexes[0] == 1);
        CHECK(candidate.pixels.paletteIndexes[7] == 2);
        CHECK(candidate.pixels.paletteIndexes[9] == 3);
        CHECK(candidate.pixels.paletteIndexes[14] == 4);
        CHECK(candidate.pixels.paletteIndexes[18] == 5);
        CHECK(candidate.pixels.paletteIndexes[21] == 6);
        CHECK(candidate.pixels.paletteIndexes[42] == 7);
        CHECK(candidate.pixels.paletteIndexes[45] == 2);
        CHECK(candidate.pixels.paletteIndexes[49] == 5);
        CHECK(candidate.pixels.paletteIndexes[54] == 8);
        CHECK(candidate.pixels.paletteIndexes[56] == 6);
        CHECK(candidate.pixels.paletteIndexes[63] == 7);
    }

    SUBCASE("case: hFlip and vFlip") {
        porytiles::NormalizedTile candidate = porytiles::candidate(config, tile, true, true);
        CHECK(candidate.palette.size == 9);
        CHECK(candidate.palette.colors[0] == porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA));
        CHECK(candidate.palette.colors[1] == porytiles::rgbaToBgr(porytiles::RGBA_BLUE));
        CHECK(candidate.palette.colors[2] == porytiles::rgbaToBgr(porytiles::RGBA_GREY));
        CHECK(candidate.palette.colors[3] == porytiles::rgbaToBgr(porytiles::RGBA_CYAN));
        CHECK(candidate.palette.colors[4] == porytiles::rgbaToBgr(porytiles::RGBA_BLACK));
        CHECK(candidate.palette.colors[5] == porytiles::rgbaToBgr(porytiles::RGBA_RED));
        CHECK(candidate.palette.colors[6] == porytiles::rgbaToBgr(porytiles::RGBA_GREEN));
        CHECK(candidate.palette.colors[7] == porytiles::rgbaToBgr(porytiles::RGBA_YELLOW));
        CHECK(candidate.palette.colors[8] == porytiles::rgbaToBgr(porytiles::RGBA_WHITE));
        CHECK(candidate.pixels.paletteIndexes[0] == 1);
        CHECK(candidate.pixels.paletteIndexes[7] == 2);
        CHECK(candidate.pixels.paletteIndexes[9] == 3);
        CHECK(candidate.pixels.paletteIndexes[14] == 4);
        CHECK(candidate.pixels.paletteIndexes[18] == 5);
        CHECK(candidate.pixels.paletteIndexes[21] == 6);
        CHECK(candidate.pixels.paletteIndexes[42] == 1);
        CHECK(candidate.pixels.paletteIndexes[45] == 7);
        CHECK(candidate.pixels.paletteIndexes[49] == 8);
        CHECK(candidate.pixels.paletteIndexes[54] == 6);
        CHECK(candidate.pixels.paletteIndexes[56] == 7);
        CHECK(candidate.pixels.paletteIndexes[63] == 5);
    }
}
