#include "emitter.h"

#include <iostream>
#include <sstream>

#include "types.h"
#include "importer.h"
#include "compiler.h"

namespace porytiles {

constexpr size_t TILES_PNG_WIDTH_IN_TILES = 16;

void emitPalette(const Config& config, std::ostream& out, const GBAPalette& palette) {
    out << "JASC-PAL" << std::endl;
    out << "0100" << std::endl;
    out << "16" << std::endl;
    size_t i;
    for (i = 0; i < palette.colors.size(); i++) {
        RGBA32 color = bgrToRgba(palette.colors.at(i));
        out << color.jasc() << std::endl;
    }
}

void emitTilesPng(const Config& config, png::image<png::index_pixel>& out, const CompiledTileset& tileset) {
    std::array<RGBA32, PAL_SIZE> greyscalePalette = {
        RGBA32{0, 0, 0, 255},
        RGBA32{16, 16, 16, 255},
        RGBA32{32, 32, 32, 255},
        RGBA32{48, 48, 48, 255},
        RGBA32{64, 64, 64, 255},
        RGBA32{80, 80, 80, 255},
        RGBA32{96, 96, 96, 255},
        RGBA32{112, 112, 112, 255},
        RGBA32{128, 128, 128, 255},
        RGBA32{144, 144, 144, 255},
        RGBA32{160, 160, 160, 255},
        RGBA32{176, 176, 176, 255},
        RGBA32{192, 192, 192, 255},
        RGBA32{208, 208, 208, 255},
        RGBA32{224, 224, 224, 255},
        RGBA32{240, 240, 240, 255}
    };

    /*
     * For the PNG palette, we'll pack all the tileset palettes into the final PNG. Since gbagfx ignores the top 4 bits
     * of an 8bpp PNG, we can use those bits to select between the various tileset palettes. That way, the final tileset
     * PNG will visually show all the correct colors while also being properly 4bpp indexed. Alternatively, we will just
     * use the colors from one of the final palettes for the whole PNG. It doesn't matter since as long as the indexes
     * are correct, it will work in-game.
     */
    // 0 initial length here since we will push_back our colors in-order
    png::palette pngPal{0};
    if (config.tilesPng8bpp) {
        for (const auto& palette: tileset.palettes) {
            for (const auto& color: palette.colors) {
                RGBA32 rgbaColor = bgrToRgba(color);
                pngPal.push_back(png::color{rgbaColor.red, rgbaColor.green, rgbaColor.blue});
            }
        }
    }
    else {
        // TODO : provide user option to select palette based on output palettes
        // for (const auto& color: tileset.palettes[config.numPalettesInPrimary - 1].colors) {
        //     RGBA32 rgbaColor = bgrToRgba(color);
        //     pngPal.push_back(png::color{rgbaColor.red, rgbaColor.green, rgbaColor.blue});
        // }
        for (const auto& color: greyscalePalette) {
            pngPal.push_back(png::color{color.red, color.green, color.blue});
        }
    }
    out.set_palette(pngPal);

    /*
     * Set up the tileset PNG based on the tiles list and then write it to `tiles.png`.
     */
    std::size_t pngWidthInTiles = out.get_width() / TILE_SIDE_LENGTH;
    std::size_t pngHeightInTiles = out.get_height() / TILE_SIDE_LENGTH;
    std::size_t tilesetTileIndex = 0;
    for (size_t tileIndex = 0; tileIndex < pngWidthInTiles * pngHeightInTiles; tileIndex++) {
        size_t tileRow = tileIndex / pngWidthInTiles;
        size_t tileCol = tileIndex % pngWidthInTiles;
        for (size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
            size_t pixelRow = (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
            size_t pixelCol = (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
            if (tileIndex < tileset.tiles.size()) {
                const GBATile& tile = tileset.tiles[tilesetTileIndex];
                png::byte paletteIndex = tileset.paletteIndexes[tilesetTileIndex];
                png::byte indexInPalette = tile.getPixel(pixelIndex);
                out[pixelRow][pixelCol] = config.tilesPng8bpp ? (paletteIndex << 4) | indexInPalette :
                                                                 indexInPalette;
            }
            else {
                // Pad out transparent tiles at end of last tiles.png row
                out[pixelRow][pixelCol] = 0;
            }
        }
        tilesetTileIndex++;
    }
}

}

// --------------------
// |    TEST CASES    |
// --------------------

TEST_CASE("emitGBAPalette should write the expected JASC pal to the output stream") {
    porytiles::Config config{};
    porytiles::GBAPalette palette{};
    palette.colors[0] = porytiles::rgbaToBgr(porytiles::RGBA_MAGENTA);
    palette.colors[1] = porytiles::rgbaToBgr(porytiles::RGBA_RED);
    palette.colors[2] = porytiles::rgbaToBgr(porytiles::RGBA_GREEN);
    palette.colors[3] = porytiles::rgbaToBgr(porytiles::RGBA_BLUE);
    palette.colors[4] = porytiles::rgbaToBgr(porytiles::RGBA_WHITE);

    std::string expectedOutput =
        "JASC-PAL\n"
        "0100\n"
        "16\n"
        "248 0 248\n"
        "248 0 0\n"
        "0 248 0\n"
        "0 0 248\n"
        "248 248 248\n"
        "0 0 0\n"
        "0 0 0\n"
        "0 0 0\n"
        "0 0 0\n"
        "0 0 0\n"
        "0 0 0\n"
        "0 0 0\n"
        "0 0 0\n"
        "0 0 0\n"
        "0 0 0\n"
        "0 0 0\n";

    std::stringstream outputStream;
    porytiles::emitPalette(config, outputStream, palette);

    CHECK(outputStream.str() == expectedOutput);
}

TEST_CASE("emitTilesPng should emit the tiles.png as expected based on settings") {
    porytiles::Config config{};
    config.transparencyColor = porytiles::RGBA_MAGENTA;
    config.numPalettesInPrimary = 5;
    config.tilesPng8bpp = true;

    REQUIRE(std::filesystem::exists("res/tests/primary_set.png"));
    png::image<png::rgba_pixel> png1{"res/tests/primary_set.png"};
    porytiles::DecompiledTileset tiles = porytiles::importRawTilesFromPng(png1);
    porytiles::CompiledTileset compiledTiles = porytiles::compile(config, tiles);

    const size_t imageWidth = porytiles::TILE_SIDE_LENGTH * porytiles::TILES_PNG_WIDTH_IN_TILES;
    const size_t imageHeight = porytiles::TILE_SIDE_LENGTH * ((compiledTiles.tiles.size() / porytiles::TILES_PNG_WIDTH_IN_TILES) + 1);
    png::image<png::index_pixel> tilesetPng{static_cast<png::uint_32>(imageWidth),
                                            static_cast<png::uint_32>(imageHeight)};

    porytiles::emitTilesPng(config, tilesetPng, compiledTiles);
    tilesetPng.write("/Users/dad/Desktop/foo.png");
}
