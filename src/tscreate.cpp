#include "tscreate.h"

#include "png_checks.h"
#include "cli_parser.h"
#include "tsexception.h"

#include <iostream>
#include <getopt.h>
#include <png.hpp>
#include <filesystem>

namespace tscreate {
const png::uint_32 TILE_DIMENSION = 8;
const png::uint_32 PAL_SIZE_4BPP = 16;
const png::uint_32 NUM_BG_PALS = 16;

std::string errorPrefix() {
    std::string program(PROGRAM_NAME);
    return program + ": error: ";
}
}

int main(int argc, char** argv) try {
    // Parse CLI options and args, fills out global opt vars with expected values
    tscreate::parseOptions(argc, argv);

    // Verifies that master PNG path points at a valid PNG file
    tscreate::validateMasterPngIsAPng(tscreate::gArgMasterPngPath);

    png::image<png::rgb_pixel> masterPng(tscreate::gArgMasterPngPath);

    // Verifies that master PNG exists and validates its dimensions (must be divisible by 8 to hold tiles)
    tscreate::validateMasterPngDimensions(masterPng);

    // Verifies that no individual tile in the master PNG has more than 16 colors
    tscreate::validateMasterPngTilesEach16Colors(masterPng);

    // Verifies that the master PNG does not have too many total unique colors
    tscreate::validateMasterPngMaxUniqueColors(masterPng);

    // Create output directory if possible, otherwise fail
    // std::filesystem::create_directory(tscreate::gArgOutputPath);

    // TODO : remove this test code that simply writes the master PNG to output dir
    // std::filesystem::path parentDirectory(tscreate::gArgOutputPath);
    // std::filesystem::path outputFile("output.png");
    // png::image<png::rgb_pixel> masterPng(tscreate::gArgMasterPngPath);
    // masterPng.write(parentDirectory / outputFile);

    return 0;
}
catch (tscreate::TsException& e) {
    std::cerr << tscreate::errorPrefix() << e.what() << std::endl;
    return 1;
}
catch (std::exception& e) {
    // TODO : if this catches, something we really didn't expect happened, can we print a stack trace here? How?
    std::cerr << tscreate::errorPrefix() << e.what() << std::endl;
    return 1;
}
