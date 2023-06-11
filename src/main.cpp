#include "png_checks.h"
#include "cli_parser.h"
#include "tsexception.h"
#include "rgb_tiled_png.h"
#include "tileset.h"
#include "palette.h"

#include <iostream>
#include <png.hpp>

namespace porytiles {
std::string errorPrefix() {
    std::string program(PROGRAM_NAME);
    return program + ": error: ";
}

std::string fatalPrefix() {
    std::string program(PROGRAM_NAME);
    return program + ": fatal: ";
}
} // namespace porytiles

int main(int argc, char** argv) try {
    // Parse CLI options and args, fill out global opt vars with expected values
    porytiles::parseOptions(argc, argv);

    // Verify that master PNG path points at a valid PNG file
    porytiles::validateMasterPngIsAPng(porytiles::gArgMasterPngPath);

    // Validate master PNG dimensions (must be divisible by 8 to hold tiles)
    png::image<png::rgb_pixel> masterPng{porytiles::gArgMasterPngPath};
    porytiles::validateMasterPngDimensions(masterPng);

    // Master PNG file is safe to tile-ize
    porytiles::RgbTiledPng masterTiles{masterPng};

    // Verify that no individual tile in the master has more than 16 colors
    porytiles::validateMasterPngTilesEach16Colors(masterTiles);

    // Verify that the master does not have too many unique colors
    porytiles::validateMasterPngMaxUniqueColors(masterTiles);

    // Build the tileset
    porytiles::Tileset tileset{porytiles::gOptMaxPalettes};
    tileset.alignSiblings(masterTiles);
    tileset.buildPalettes(masterTiles);
    tileset.indexTiles(masterTiles);

    return 0;
}
catch (const porytiles::TsException& e) {
    // Catch TsException here, these are errors that can reasonably be expected due to bad input, bad files, etc
    std::cerr << porytiles::errorPrefix() << e.what() << std::endl;
    return 1;
}
catch (const std::exception& e) {
    // TODO : if this catches, something we really didn't expect happened, can we print a stack trace here? How?
    std::cerr << porytiles::fatalPrefix() << e.what() << std::endl;
    std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
              << std::endl;
    std::cerr
            << "This is a bug. Please file an issue here: https://github.com/grunt-lucas/porytiles/issues"
            << std::endl;
    std::cerr << "Be sure to include the full command you ran, as well as any accompanying input files that"
              << std::endl;
    std::cerr << "trigger the error. This way a maintainer can reproduce the issue." << std::endl;
    return 1;
}

void testTileFlip() {
    porytiles::IndexedTile t1;
    for (size_t i = 0; i < 64; i++) {
        t1.setPixel(i, i);
    }
    for (size_t row = 0; row < porytiles::TILE_DIMENSION; row++) {
        for (size_t col = 0; col < porytiles::TILE_DIMENSION; col++) {
            std::cout << std::to_string(t1.getPixel(row, col)) << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "----------------------------------------" << std::endl;

    porytiles::IndexedTile verticalFlip = t1.getVerticalFlip();
    for (size_t row = 0; row < porytiles::TILE_DIMENSION; row++) {
        for (size_t col = 0; col < porytiles::TILE_DIMENSION; col++) {
            std::cout << std::to_string(verticalFlip.getPixel(row, col)) << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "----------------------------------------" << std::endl;

    porytiles::IndexedTile horizontalFlip = t1.getHorizontalFlip();
    for (size_t row = 0; row < porytiles::TILE_DIMENSION; row++) {
        for (size_t col = 0; col < porytiles::TILE_DIMENSION; col++) {
            std::cout << std::to_string(horizontalFlip.getPixel(row, col)) << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "----------------------------------------" << std::endl;

    porytiles::IndexedTile bothFlip = t1.getHorizontalFlip().getVerticalFlip();
    for (size_t row = 0; row < porytiles::TILE_DIMENSION; row++) {
        for (size_t col = 0; col < porytiles::TILE_DIMENSION; col++) {
            std::cout << std::to_string(bothFlip.getPixel(row, col)) << " ";
        }
        std::cout << std::endl;
    }
}
