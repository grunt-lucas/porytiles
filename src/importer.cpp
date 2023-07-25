#include "importer.h"

#include <iostream>
#include <png.hpp>

#include "types.h"
#include "ptexception.h"

namespace porytiles {

DecompiledTileset importRawTilesFromPng(const png::image<png::rgba_pixel>& png) {
    if (png.get_height() % TILE_SIDE_LENGTH != 0) {
        throw PtException{"input PNG height `" + std::to_string(png.get_height()) + "' is not divisible by 8"};
    }
    if (png.get_width() % TILE_SIDE_LENGTH != 0) {
        throw PtException{"input PNG width `" + std::to_string(png.get_width()) + "' is not divisible by 8"};
    }

    DecompiledTileset decompiledTiles;

    std::size_t pngWidthInTiles = png.get_width() / TILE_SIDE_LENGTH;
    std::size_t pngHeightInTiles = png.get_height() / TILE_SIDE_LENGTH;

    for (size_t tileIndex = 0; tileIndex < pngWidthInTiles * pngHeightInTiles; tileIndex++) {
        size_t tileRow = tileIndex / pngWidthInTiles;
        size_t tileCol = tileIndex % pngWidthInTiles;
        RGBATile tile{};
        for (size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
            size_t pixelRow = (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
            size_t pixelCol = (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
            tile.pixels[pixelIndex].red = png[pixelRow][pixelCol].red;
            tile.pixels[pixelIndex].green = png[pixelRow][pixelCol].green;
            tile.pixels[pixelIndex].blue = png[pixelRow][pixelCol].blue;
            tile.pixels[pixelIndex].alpha = png[pixelRow][pixelCol].alpha;
        }
        decompiledTiles.tiles.push_back(tile);
    }
    return decompiledTiles;
}

DecompiledTileset
importLayeredTilesFromPngs(const png::image<png::rgba_pixel>& bottom, const png::image<png::rgba_pixel>& middle, const png::image<png::rgba_pixel>& top) {
    if (bottom.get_height() % METATILE_SIDE_LENGTH != 0) {
        throw PtException{"bottom layer input PNG height `" + std::to_string(bottom.get_height()) + "' is not divisible by 16"};
    }
    if (middle.get_height() % METATILE_SIDE_LENGTH != 0) {
        throw PtException{"middle layer input PNG height `" + std::to_string(middle.get_height()) + "' is not divisible by 16"};
    }
    if (top.get_height() % METATILE_SIDE_LENGTH != 0) {
        throw PtException{"top layer input PNG height `" + std::to_string(top.get_height()) + "' is not divisible by 16"};
    }
    if (bottom.get_width() != METATILE_SIDE_LENGTH * METATILES_IN_ROW) {
        throw PtException{"bottom layer input PNG width `" + std::to_string(bottom.get_width()) + "' was not 128"};
    }
    if (middle.get_width() != METATILE_SIDE_LENGTH * METATILES_IN_ROW) {
        throw PtException{"middle layer input PNG width `" + std::to_string(middle.get_width()) + "' was not 128"};
    }
    if (top.get_width() != METATILE_SIDE_LENGTH * METATILES_IN_ROW) {
        throw PtException{"top layer input PNG width `" + std::to_string(top.get_width()) + "' was not 128"};
    }
    if ((bottom.get_height() != middle.get_height()) || (bottom.get_height() != top.get_height())) {
        throw PtException{"layer input PNG heights `" + std::to_string(bottom.get_height()) + "," +
                                                        std::to_string(middle.get_height()) + "," +
                                                        std::to_string(top.get_height()) + "' must be equivalent"};
    }

    DecompiledTileset decompiledTiles;

    // Since all widths and heights are the same, we can just read the bottom layer's width and height
    std::size_t widthInMetatiles = bottom.get_width() / METATILE_SIDE_LENGTH;
    std::size_t heightInMetatiles = bottom.get_height() / METATILE_SIDE_LENGTH;

    for (size_t metatileIndex = 0; metatileIndex < widthInMetatiles * heightInMetatiles; metatileIndex++) {
        size_t metatileRow = metatileIndex / widthInMetatiles;
        size_t metatileCol = metatileIndex % widthInMetatiles;
        for (size_t bottomTileIndex = 0; bottomTileIndex < METATILE_TILE_SIDE_LENGTH * METATILE_TILE_SIDE_LENGTH; bottomTileIndex++) {
            size_t tileRow = bottomTileIndex / METATILE_TILE_SIDE_LENGTH;
            size_t tileCol = bottomTileIndex % METATILE_TILE_SIDE_LENGTH;
            RGBATile bottomTile{};
            for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
                size_t pixelRow = (metatileRow * METATILE_SIDE_LENGTH) + (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
                size_t pixelCol = (metatileCol * METATILE_SIDE_LENGTH) + (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
                bottomTile.pixels[pixelIndex].red = bottom[pixelRow][pixelCol].red;
                bottomTile.pixels[pixelIndex].green = bottom[pixelRow][pixelCol].green;
                bottomTile.pixels[pixelIndex].blue = bottom[pixelRow][pixelCol].blue;
                bottomTile.pixels[pixelIndex].alpha = bottom[pixelRow][pixelCol].alpha;
            }
            decompiledTiles.tiles.push_back(bottomTile);
        }
        for (size_t middleTileIndex = 0; middleTileIndex < METATILE_TILE_SIDE_LENGTH * METATILE_TILE_SIDE_LENGTH; middleTileIndex++) {
            size_t tileRow = middleTileIndex / METATILE_TILE_SIDE_LENGTH;
            size_t tileCol = middleTileIndex % METATILE_TILE_SIDE_LENGTH;
            RGBATile middleTile{};
            for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
                size_t pixelRow = (metatileRow * METATILE_SIDE_LENGTH) + (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
                size_t pixelCol = (metatileCol * METATILE_SIDE_LENGTH) + (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
                middleTile.pixels[pixelIndex].red = middle[pixelRow][pixelCol].red;
                middleTile.pixels[pixelIndex].green = middle[pixelRow][pixelCol].green;
                middleTile.pixels[pixelIndex].blue = middle[pixelRow][pixelCol].blue;
                middleTile.pixels[pixelIndex].alpha = middle[pixelRow][pixelCol].alpha;
            }
            decompiledTiles.tiles.push_back(middleTile);
        }
        for (size_t topTileIndex = 0; topTileIndex < METATILE_TILE_SIDE_LENGTH * METATILE_TILE_SIDE_LENGTH; topTileIndex++) {
            size_t tileRow = topTileIndex / METATILE_TILE_SIDE_LENGTH;
            size_t tileCol = topTileIndex % METATILE_TILE_SIDE_LENGTH;
            RGBATile topTile{};
            for (std::size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
                size_t pixelRow = (metatileRow * METATILE_SIDE_LENGTH) + (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
                size_t pixelCol = (metatileCol * METATILE_SIDE_LENGTH) + (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
                topTile.pixels[pixelIndex].red = top[pixelRow][pixelCol].red;
                topTile.pixels[pixelIndex].green = top[pixelRow][pixelCol].green;
                topTile.pixels[pixelIndex].blue = top[pixelRow][pixelCol].blue;
                topTile.pixels[pixelIndex].alpha = top[pixelRow][pixelCol].alpha;
            }
            decompiledTiles.tiles.push_back(topTile);
        }
    }

    return decompiledTiles;
}

}

TEST_CASE("importRawTilesFromPng should read an RGBA PNG into a DecompiledTileset in tile-wise left-to-right, top-to-bottom order") {
    REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_1.png"));
    png::image<png::rgba_pixel> png1{"res/tests/2x2_pattern_1.png"};

    porytiles::DecompiledTileset tiles = porytiles::importRawTilesFromPng(png1);

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

TEST_CASE("importLayeredTilesFromPngs should read the RGBA PNGs into a DecompiledTileset in correct metatile order") {
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_1/bottom.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_1/middle.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_1/top.png"));

    png::image<png::rgba_pixel> bottom{"res/tests/simple_metatiles_1/bottom.png"};
    png::image<png::rgba_pixel> middle{"res/tests/simple_metatiles_1/middle.png"};
    png::image<png::rgba_pixel> top{"res/tests/simple_metatiles_1/top.png"};

    porytiles::DecompiledTileset tiles = porytiles::importLayeredTilesFromPngs(bottom, middle, top);

    // Metatile 0 bottom layer
    CHECK(tiles.tiles[0] == porytiles::RGBA_TILE_RED);
    CHECK(tiles.tiles[1] == porytiles::RGBA_TILE_MAGENTA);
    CHECK(tiles.tiles[2] == porytiles::RGBA_TILE_MAGENTA);
    CHECK(tiles.tiles[3] == porytiles::RGBA_TILE_YELLOW);

    // Metatile 0 middle layer
    CHECK(tiles.tiles[4] == porytiles::RGBA_TILE_MAGENTA);
    CHECK(tiles.tiles[5] == porytiles::RGBA_TILE_MAGENTA);
    CHECK(tiles.tiles[6] == porytiles::RGBA_TILE_GREEN);
    CHECK(tiles.tiles[7] == porytiles::RGBA_TILE_MAGENTA);

    // Metatile 0 top layer
    CHECK(tiles.tiles[8] == porytiles::RGBA_TILE_MAGENTA);
    CHECK(tiles.tiles[9] == porytiles::RGBA_TILE_BLUE);
    CHECK(tiles.tiles[10] == porytiles::RGBA_TILE_MAGENTA);
    CHECK(tiles.tiles[11] == porytiles::RGBA_TILE_MAGENTA);
}
