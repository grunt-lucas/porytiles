#include "png_frontend.h"

#include <png.hpp>

#include "types.h"

namespace porytiles {
DecompiledTileset decompiledTilesFrom(const png::image<png::rgb_pixel>& png) {
    /*
     * Preconditions:
     * 1) input png width is divisible by 8
     * 2) input png height is divisible by 8 and not larger than max allowed metatiles
     *
     * These must be enforced at the callsite
     *
     * TODO : enforce these preconditions at callsite
     */
    DecompiledTileset decompiledTiles;

    std::size_t pngWidthInTiles = png.get_width() / TILE_SIDE_LENGTH;
    std::size_t pngHeightInTiles = png.get_height() / TILE_SIDE_LENGTH;

    for (size_t tileIndex = 0; tileIndex < pngWidthInTiles * pngHeightInTiles; tileIndex++) {
        size_t tileRow = tileIndex / pngWidthInTiles;
        size_t tileCol = tileIndex % pngWidthInTiles;
        std::cout << "tile col,row = " << tileCol << "," << tileRow << std::endl;
        std::cout << "-----------" << std::endl;
        for (size_t pixelIndex = 0; pixelIndex < TILE_NUM_PIX; pixelIndex++) {
            size_t pixelRow = (tileRow * TILE_SIDE_LENGTH) + (pixelIndex / TILE_SIDE_LENGTH);
            size_t pixelCol = (tileCol * TILE_SIDE_LENGTH) + (pixelIndex % TILE_SIDE_LENGTH);
            std::cout << "pixel col,row = " << pixelCol << "," << pixelRow << std::endl;
        }
        std::cout << "======================================================" << std::endl;
    }

    return decompiledTiles;
}
}

// TODO : name this test
TEST_CASE("png_frontend TODO name test") {
    REQUIRE(std::filesystem::exists("res/tests/2x2_pattern_1.png"));
    png::image<png::rgb_pixel> png1{"res/tests/2x2_pattern_1.png"};

    porytiles::decompiledTilesFrom(png1);
}
