#include "emitter.h"

#include <iostream>
#include <sstream>

#include "types.h"

namespace porytiles {

// constexpr size_t TILES_PNG_WIDTH_IN_TILES = 16;

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

void emitTilesPng(const Config& config, const png::image<png::rgba_pixel>& out, const CompiledTileset& tileset) {
    // const size_t imageWidth = TILE_SIDE_LENGTH * TILES_PNG_WIDTH_IN_TILES;
    // const size_t imageHeight = TILE_SIDE_LENGTH * (tileset.tiles.size() / TILES_PNG_WIDTH_IN_TILES);
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