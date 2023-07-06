#include "compiler.h"

#include "doctest.h"
#include "config.h"
#include "ptexception.h"
#include "constants.h"
#include "types.h"

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
        auto bgr = rgbaToBGR(rgba);
        auto bgrPosInPalette = std::find(std::begin(palette.colors) + 1, std::begin(palette.colors) + palette.size, bgr);
        auto selectedIndex = bgrPosInPalette - std::begin(palette.colors);
        if (selectedIndex == palette.size) {
            // palette size will grow as we add to it
            if (palette.size == PAL_SIZE) {
                // TODO : better error context
                throw PtException{"too many colors"};
            }
            palette.colors[palette.size++] = bgr;
        }
        return selectedIndex;
    }
    throw PtException{"invalid alpha value: " + std::to_string(rgba.alpha)};
}
TEST_CASE("TODO : name this test") {
    Config config;
    config.transparencyColor = RGBA_MAGENTA;

    NormalizedPalette palette1;
    palette1.size = 1;
    palette1.colors = {};

    CHECK(insertRGBA(config, palette1, RGBA_BLACK) == 1);
    CHECK(insertRGBA(config, palette1, RGBA_WHITE) == 2);
    CHECK(insertRGBA(config, palette1, RGBA_RED) == 3);
    CHECK(insertRGBA(config, palette1, RGBA_BLACK) == 1);
    CHECK(insertRGBA(config, palette1, RGBA_MAGENTA) == 0);
    CHECK(insertRGBA(config, palette1, RGBA32{0, 0, 0, 0}) == 0);
    CHECK_THROWS_WITH_AS(insertRGBA(config, palette1, RGBA32{0, 0, 0, 12}),
                             "invalid alpha value: 12",
                             const PtException&);
}

}
