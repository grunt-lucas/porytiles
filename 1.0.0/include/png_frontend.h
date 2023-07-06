#ifndef PORYTILES_PNG_FRONTEND_H
#define PORYTILES_PNG_FRONTEND_H

#include <filesystem>
#include <png.hpp>

#include "types.h"

/**
 * TODO : fill in doc comment for this header
 */

namespace porytiles {
/**
 * Preconditions:
 * 1) input png width is divisible by 8
 * 2) input png height is divisible by 8 and not larger than max allowed metatiles
 *
 * These must be enforced at the callsite
 *
 * TODO : enforce these preconditions at callsite
 */
DecompiledTileset importTilesFrom(const png::image<png::rgba_pixel>& png);
}

TEST_CASE("png_frontend::decompiledTilesFrom should read an RGBA PNG into a DecompiledTileset in tile-wise left-to-right, top-to-bottom order") {
    REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_1.png"));
    png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_1.png"};

    porytiles::DecompiledTileset tiles = porytiles::importTilesFrom(png1);

    // Tile 0 should have blue stripe from top left to bottom right
    CHECK(tiles.tiles[0].pixels[0] == porytiles::RGBA_BLUE);
    CHECK(tiles.tiles[0].pixels[9] == porytiles::RGBA_BLUE);
    CHECK(tiles.tiles[0].pixels[54] == porytiles::RGBA_BLUE);
    CHECK(tiles.tiles[0].pixels[63] == porytiles::RGBA_BLUE);
    CHECK(tiles.tiles[0].pixels[1] == porytiles::RGBA_MAGENTA);

    // Tile 1 should have red stripe from top right to bottom left
    CHECK(tiles.tiles[1].pixels[7] == porytiles::RGBA_RED);
    CHECK(tiles.tiles[1].pixels[14] == porytiles::RGBA_RED);
    CHECK(tiles.tiles[1].pixels[49] == porytiles::RGBA_RED);
    CHECK(tiles.tiles[1].pixels[56] == porytiles::RGBA_RED);
    CHECK(tiles.tiles[1].pixels[0] == porytiles::RGBA_MAGENTA);

    // Tile 2 should have green stripe from top right to bottom left
    CHECK(tiles.tiles[2].pixels[7] == porytiles::RGBA_GREEN);
    CHECK(tiles.tiles[2].pixels[14] == porytiles::RGBA_GREEN);
    CHECK(tiles.tiles[2].pixels[49] == porytiles::RGBA_GREEN);
    CHECK(tiles.tiles[2].pixels[56] == porytiles::RGBA_GREEN);
    CHECK(tiles.tiles[2].pixels[0] == porytiles::RGBA_MAGENTA);

    // Tile 3 should have yellow stripe from top left to bottom right
    CHECK(tiles.tiles[3].pixels[0] == porytiles::RGBA_YELLOW);
    CHECK(tiles.tiles[3].pixels[9] == porytiles::RGBA_YELLOW);
    CHECK(tiles.tiles[3].pixels[54] == porytiles::RGBA_YELLOW);
    CHECK(tiles.tiles[3].pixels[63] == porytiles::RGBA_YELLOW);
    CHECK(tiles.tiles[3].pixels[1] == porytiles::RGBA_MAGENTA);
}

#endif