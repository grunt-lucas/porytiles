#include "emitter.h"

#include <iostream>
#include <sstream>

#include "types.h"

namespace porytiles {

void emitGBAPalette(std::ostream& out, const GBAPalette& palette) {
    out << "JASC-PAL" << std::endl;
    out << "0100" << std::endl;
    out << "16" << std::endl;
    size_t i;
    for (i = 0; i < palette.colors.size(); i++) {
        RGBA32 color = bgrToRgba(palette.colors.at(i));
        out << color.jasc() << std::endl;
    }
}

}

// --------------------
// |    TEST CASES    |
// --------------------

TEST_CASE("emitGBAPalette should write the expected JASC pal to the output stream") {
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
    porytiles::emitGBAPalette(outputStream, palette);

    CHECK(outputStream.str() == expectedOutput);
}
