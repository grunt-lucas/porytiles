#include "emitter.h"

#include <iostream>
#include <sstream>
#include <filesystem>

#include "types.h"
#include "importer.h"
#include "compiler.h"
#include "tmpfiles.h"

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

void emitZeroedPalette(const Config& config, std::ostream& out) {
    GBAPalette palette{};
    palette.colors[0] = rgbaToBgr(config.transparencyColor);
    emitPalette(config, out, palette);
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
     * use the colors from one of the final palettes for the whole PNG. It doesn't matter since as long as the least
     * significant 4 bit indexes are correct, it will work in-game.
     */
    // 0 initial length here since we will push_back our colors in-order
    png::palette pngPal{0};
    if (config.tilesPngPaletteMode == TRUE_COLOR) {
        for (const auto& palette: tileset.palettes) {
            for (const auto& color: palette.colors) {
                RGBA32 rgbaColor = bgrToRgba(color);
                pngPal.push_back(png::color{rgbaColor.red, rgbaColor.green, rgbaColor.blue});
            }
        }
    }
    else if (config.tilesPngPaletteMode == GREYSCALE) {
        for (const auto& color: greyscalePalette) {
            pngPal.push_back(png::color{color.red, color.green, color.blue});
        }
    }
    else {
        // TODO : provide user option to select palette based on output palettes
        // for (const auto& color: tileset.palettes[config.numPalettesInPrimary - 1].colors) {
        //     RGBA32 rgbaColor = bgrToRgba(color);
        //     pngPal.push_back(png::color{rgbaColor.red, rgbaColor.green, rgbaColor.blue});
        // }
        throw std::runtime_error{"PAL0 option not yet supported"};
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
                png::byte paletteIndex = tileset.paletteIndexesOfTile[tilesetTileIndex];
                png::byte indexInPalette = tile.getPixel(pixelIndex);
                switch(config.tilesPngPaletteMode) {
                    case PAL0:
                    case GREYSCALE:
                        out[pixelRow][pixelCol] = indexInPalette;
                        break;
                    case TRUE_COLOR:
                        out[pixelRow][pixelCol] = (paletteIndex << 4) | indexInPalette;
                }
            }
            else {
                // Pad out transparent tiles at end of last tiles.png row
                out[pixelRow][pixelCol] = 0;
            }
        }
        tilesetTileIndex++;
    }
}

void emitMetatilesBin(const Config& config, std::ostream& out, const CompiledTileset& tileset) {
    for(std::size_t i = 0; i < tileset.assignments.size(); i++) {
        auto& assignment = tileset.assignments.at(i);
        uint16_t tileValue = static_cast<uint16_t>(
               (assignment.tileIndex & 0x3FF)
            | ((assignment.hFlip & 1) << 10)
            | ((assignment.vFlip & 1) << 11)
            | ((assignment.paletteIndex & 0xF) << 12)
        );
        out << static_cast<char>(tileValue);
        out << static_cast<char>(tileValue >> 8);
    }
    out.flush();
}

}

// --------------------
// |    TEST CASES    |
// --------------------

TEST_CASE("emitPalette should write the expected JASC pal to the output stream") {
    porytiles::Config config = porytiles::defaultConfig();
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

TEST_CASE("emitZeroedPalette should write the expected JASC pal to the output stream") {
    porytiles::Config config = porytiles::defaultConfig();

    std::string expectedOutput =
        "JASC-PAL\n"
        "0100\n"
        "16\n"
        "248 0 248\n"
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
        "0 0 0\n"
        "0 0 0\n"
        "0 0 0\n"
        "0 0 0\n"
        "0 0 0\n";

    std::stringstream outputStream;
    porytiles::emitZeroedPalette(config, outputStream);

    CHECK(outputStream.str() == expectedOutput);
}

TEST_CASE("emitTilesPng should emit the tiles.png as expected based on settings") {
    porytiles::Config config = porytiles::defaultConfig();

    REQUIRE(std::filesystem::exists("res/tests/compile_raw_set_1/set.png"));
    REQUIRE(std::filesystem::exists("res/tests/compile_raw_set_1/expected.png"));

    png::image<png::rgba_pixel> png1{"res/tests/compile_raw_set_1/set.png"};
    porytiles::DecompiledTileset tiles = porytiles::importRawTilesFromPng(png1);
    porytiles::CompiledTileset compiledTiles = porytiles::compilePrimary(config, tiles);

    const size_t imageWidth = porytiles::TILE_SIDE_LENGTH * porytiles::TILES_PNG_WIDTH_IN_TILES;
    const size_t imageHeight = porytiles::TILE_SIDE_LENGTH * ((compiledTiles.tiles.size() / porytiles::TILES_PNG_WIDTH_IN_TILES) + 1);
    png::image<png::index_pixel> outPng{static_cast<png::uint_32>(imageWidth),
                                            static_cast<png::uint_32>(imageHeight)};

    porytiles::emitTilesPng(config, outPng, compiledTiles);
    std::filesystem::path pngTmpPath = porytiles::getTmpfilePath("emitTilesPng_test.png");
    outPng.write(pngTmpPath);

    png::image<png::index_pixel> tilesetPng{pngTmpPath};
    png::image<png::index_pixel> expectedPng{"res/tests/compile_raw_set_1/expected.png"};

    CHECK(tilesetPng.get_width() == expectedPng.get_width());
    CHECK(tilesetPng.get_height() == expectedPng.get_height());
    for (std::size_t pixelRow = 0; pixelRow < tilesetPng.get_height(); pixelRow++) {
        for (std::size_t pixelCol = 0; pixelCol < tilesetPng.get_width(); pixelCol++) {
            CHECK(tilesetPng[pixelRow][pixelCol] == expectedPng[pixelRow][pixelCol]);
        }
    }

    std::filesystem::remove(pngTmpPath);
}

TEST_CASE("emitMetatilesBin should emit metatiles.bin as expected based on settings") {
    porytiles::Config config = porytiles::defaultConfig();
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_1/bottom.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_1/middle.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_1/top.png"));

    png::image<png::rgba_pixel> bottom{"res/tests/simple_metatiles_1/bottom.png"};
    png::image<png::rgba_pixel> middle{"res/tests/simple_metatiles_1/middle.png"};
    png::image<png::rgba_pixel> top{"res/tests/simple_metatiles_1/top.png"};

    porytiles::DecompiledTileset decompiled = porytiles::importLayeredTilesFromPngs(bottom, middle, top);
    porytiles::CompiledTileset compiled = porytiles::compilePrimary(config, decompiled);

    std::filesystem::path tmpPath = porytiles::getTmpfilePath("emitMetatilesBin_test.bin");
    std::ofstream outFile{tmpPath};
    porytiles::emitMetatilesBin(config, outFile, compiled);
    outFile.close();

    std::ifstream input(tmpPath, std::ios::binary);
    std::vector<char> bytes(
         (std::istreambuf_iterator<char>(input)),
         (std::istreambuf_iterator<char>()));
    input.close();

    // Check metatiles.bin bytes are as expected
    CHECK(bytes[0] == 1);
    CHECK(bytes[1] == 32);
    CHECK(bytes[2] == 0);
    CHECK(bytes[3] == 0);
    CHECK(bytes[4] == 0);
    CHECK(bytes[5] == 0);
    CHECK(bytes[6] == 1);
    CHECK(bytes[7] == 48);
    CHECK(bytes[8] == 0);
    CHECK(bytes[9] == 0);
    CHECK(bytes[10] == 0);
    CHECK(bytes[11] == 0);
    CHECK(bytes[12] == 1);
    CHECK(bytes[13] == 64);
    CHECK(bytes[14] == 0);
    CHECK(bytes[15] == 0);
    CHECK(bytes[16] == 0);
    CHECK(bytes[17] == 0);
    CHECK(bytes[18] == 1);
    CHECK(bytes[19] == 80);
    for (std::size_t i = 20; i < bytes.size(); i++) {
        CHECK(bytes[i] == 0);
    }

    std::filesystem::remove(tmpPath);
}
