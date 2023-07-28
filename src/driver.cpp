#include "driver.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <exception>
#include <png.hpp>
#include <cstdio>

#include "doctest.h"
#include "config.h"
#include "emitter.h"
#include "compiler.h"
#include "importer.h"
#include "tmpfiles.h"
#include "ptexception.h"

namespace porytiles {

static void emitPalettes(const Config& config, const CompiledTileset& compiledTiles, const std::filesystem::path& palettesPath) {
    for (std::size_t i = 0; i < config.numPalettesTotal; i++) {
        std::string fileName = i < 10 ? "0" + std::to_string(i) : std::to_string(i);
        fileName += ".pal";
        std::filesystem::path paletteFile = palettesPath / fileName;
        std::ofstream outPal{paletteFile.string()};
        if (i < compiledTiles.palettes.size()) {
            emitPalette(config, outPal, compiledTiles.palettes.at(i));
        }
        else {
            emitZeroedPalette(config, outPal);
        }
        outPal.close();
    }
}

static void emitTilesPng(const Config& config, const CompiledTileset& compiledTiles, const std::filesystem::path& tilesetPath) {
    const std::size_t imageWidth = porytiles::TILE_SIDE_LENGTH * porytiles::TILES_PNG_WIDTH_IN_TILES;
    const std::size_t imageHeight = porytiles::TILE_SIDE_LENGTH * ((compiledTiles.tiles.size() / porytiles::TILES_PNG_WIDTH_IN_TILES) + 1);
    png::image<png::index_pixel> tilesPng{static_cast<png::uint_32>(imageWidth), static_cast<png::uint_32>(imageHeight)};

    emitTilesPng(config, tilesPng, compiledTiles);
    tilesPng.write(tilesetPath);
}

static void driveCompileRaw(const Config& config) {
    // if (std::filesystem::exists(config.outputPath) && !std::filesystem::is_directory(config.outputPath)) {
    //     throw PtException{config.outputPath + ": exists but is not a directory"};
    // }
    // if (config.secondary) {
    //     if (!std::filesystem::exists(config.rawSecondaryTilesheetPath)) {
    //     throw PtException{config.rawSecondaryTilesheetPath + ": file does not exist"};
    //     }
    //     if (!std::filesystem::is_regular_file(config.rawSecondaryTilesheetPath)) {
    //         throw PtException{config.rawSecondaryTilesheetPath + ": exists but was not a regular file"};
    //     }
    // }
    // if (!std::filesystem::exists(config.rawPrimaryTilesheetPath)) {
    //     throw PtException{config.rawPrimaryTilesheetPath + ": file does not exist"};
    // }
    // if (!std::filesystem::is_regular_file(config.rawPrimaryTilesheetPath)) {
    //     throw PtException{config.rawPrimaryTilesheetPath + ": exists but was not a regular file"};
    // }

    // if (config.secondary) {
    //     try {
    //     // We do this here so if the input is not a PNG, we can catch and give a better error
    //         png::image<png::rgba_pixel> tilesheetPng{config.rawSecondaryTilesheetPath};
    //     }
    //     catch(const std::exception& exception) {
    //         throw PtException{config.rawSecondaryTilesheetPath + " is not a valid PNG file"};
    //     }
    // }
    // try {
    //     // We do this here so if the input is not a PNG, we can catch and give a better error
    //     png::image<png::rgba_pixel> tilesheetPng{config.rawPrimaryTilesheetPath};
    // }
    // catch(const std::exception& exception) {
    //     throw PtException{config.rawPrimaryTilesheetPath + " is not a valid PNG file"};
    // }

    // // confirmed image was a PNG, open it again
    // png::image<png::rgba_pixel> tilesheetPng{config.rawTilesheetPath};
    // DecompiledTileset decompiledTiles = importRawTilesFromPng(tilesheetPng);
    // porytiles::CompilerContext context{config, porytiles::CompilerMode::RAW};
    // // TODO : change this over to compile once it supports RAW mode
    // CompiledTileset compiledTiles = compilePrimary(context, decompiledTiles);

    // std::filesystem::path outputPath(config.outputPath);
    // std::filesystem::path palettesDir("palettes");
    // std::filesystem::path tilesetFile("tiles.png");
    // std::filesystem::path tilesetPath = config.outputPath / tilesetFile;
    // std::filesystem::path palettesPath = config.outputPath / palettesDir;

    // if (std::filesystem::exists(tilesetPath) && !std::filesystem::is_regular_file(tilesetPath)) {
    //     throw PtException{"`" + tilesetPath.string() + "' exists in output directory but is not a file"};
    // }
    // if (std::filesystem::exists(palettesPath) && !std::filesystem::is_directory(palettesPath)) {
    //     throw PtException{"`" + palettesDir.string() + "' exists in output directory but is not a directory"};
    // }
    // std::filesystem::create_directories(palettesPath);

    // emitPalettes(config, compiledTiles, palettesPath);
    // emitTilesPng(config, compiledTiles, tilesetPath);
}

static void driveCompile(const Config& config) {
    if (std::filesystem::exists(config.outputPath) && !std::filesystem::is_directory(config.outputPath)) {
        throw PtException{config.outputPath + ": exists but is not a directory"};
    }
    if (config.secondary) {
        if (!std::filesystem::exists(config.bottomSecondaryTilesheetPath)) {
            throw PtException{config.bottomSecondaryTilesheetPath + ": file does not exist"};
        }
        if (!std::filesystem::is_regular_file(config.bottomSecondaryTilesheetPath)) {
            throw PtException{config.bottomSecondaryTilesheetPath + ": exists but was not a regular file"};
        }
        if (!std::filesystem::exists(config.middleSecondaryTilesheetPath)) {
            throw PtException{config.middleSecondaryTilesheetPath + ": file does not exist"};
        }
        if (!std::filesystem::is_regular_file(config.middleSecondaryTilesheetPath)) {
            throw PtException{config.middleSecondaryTilesheetPath + ": exists but was not a regular file"};
        }
        if (!std::filesystem::exists(config.topSecondaryTilesheetPath)) {
            throw PtException{config.topSecondaryTilesheetPath + ": file does not exist"};
        }
        if (!std::filesystem::is_regular_file(config.topSecondaryTilesheetPath)) {
            throw PtException{config.topSecondaryTilesheetPath + ": exists but was not a regular file"};
        }
    }
    if (!std::filesystem::exists(config.bottomPrimaryTilesheetPath)) {
        throw PtException{config.bottomPrimaryTilesheetPath + ": file does not exist"};
    }
    if (!std::filesystem::is_regular_file(config.bottomPrimaryTilesheetPath)) {
        throw PtException{config.bottomPrimaryTilesheetPath + ": exists but was not a regular file"};
    }
    if (!std::filesystem::exists(config.middlePrimaryTilesheetPath)) {
        throw PtException{config.middlePrimaryTilesheetPath + ": file does not exist"};
    }
    if (!std::filesystem::is_regular_file(config.middlePrimaryTilesheetPath)) {
        throw PtException{config.middlePrimaryTilesheetPath + ": exists but was not a regular file"};
    }
    if (!std::filesystem::exists(config.topPrimaryTilesheetPath)) {
        throw PtException{config.topPrimaryTilesheetPath + ": file does not exist"};
    }
    if (!std::filesystem::is_regular_file(config.topPrimaryTilesheetPath)) {
        throw PtException{config.topPrimaryTilesheetPath + ": exists but was not a regular file"};
    }

    if (config.secondary) {
        try {
            // We do this here so if the input is not a PNG, we can catch and give a better error
            png::image<png::rgba_pixel> tilesheetPng{config.bottomSecondaryTilesheetPath};
        }
        catch(const std::exception& exception) {
            throw PtException{config.bottomSecondaryTilesheetPath + " is not a valid PNG file"};
        }
        try {
            // We do this here so if the input is not a PNG, we can catch and give a better error
            png::image<png::rgba_pixel> tilesheetPng{config.middleSecondaryTilesheetPath};
        }
        catch(const std::exception& exception) {
            throw PtException{config.middleSecondaryTilesheetPath + " is not a valid PNG file"};
        }
        try {
            // We do this here so if the input is not a PNG, we can catch and give a better error
            png::image<png::rgba_pixel> tilesheetPng{config.topSecondaryTilesheetPath};
        }
        catch(const std::exception& exception) {
            throw PtException{config.topSecondaryTilesheetPath + " is not a valid PNG file"};
        }
    }
    try {
        // We do this here so if the input is not a PNG, we can catch and give a better error
        png::image<png::rgba_pixel> tilesheetPng{config.bottomPrimaryTilesheetPath};
    }
    catch(const std::exception& exception) {
        throw PtException{config.bottomPrimaryTilesheetPath + " is not a valid PNG file"};
    }
    try {
        // We do this here so if the input is not a PNG, we can catch and give a better error
        png::image<png::rgba_pixel> tilesheetPng{config.middlePrimaryTilesheetPath};
    }
    catch(const std::exception& exception) {
        throw PtException{config.middlePrimaryTilesheetPath + " is not a valid PNG file"};
    }
    try {
        // We do this here so if the input is not a PNG, we can catch and give a better error
        png::image<png::rgba_pixel> tilesheetPng{config.topPrimaryTilesheetPath};
    }
    catch(const std::exception& exception) {
        throw PtException{config.topPrimaryTilesheetPath + " is not a valid PNG file"};
    }

    CompiledTileset compiledTiles{};
    if (config.secondary) {
        png::image<png::rgba_pixel> bottomPrimaryPng{config.bottomPrimaryTilesheetPath};
        png::image<png::rgba_pixel> middlePrimaryPng{config.middlePrimaryTilesheetPath};
        png::image<png::rgba_pixel> topPrimaryPng{config.topPrimaryTilesheetPath};
        DecompiledTileset decompiledPrimaryTiles = importLayeredTilesFromPngs(bottomPrimaryPng, middlePrimaryPng, topPrimaryPng);
        porytiles::CompilerContext primaryContext{config, porytiles::CompilerMode::PRIMARY};
        CompiledTileset compiledPrimaryTiles = compile(primaryContext, decompiledPrimaryTiles);

        png::image<png::rgba_pixel> bottomPng{config.bottomSecondaryTilesheetPath};
        png::image<png::rgba_pixel> middlePng{config.middleSecondaryTilesheetPath};
        png::image<png::rgba_pixel> topPng{config.topSecondaryTilesheetPath};
        DecompiledTileset decompiledTiles = importLayeredTilesFromPngs(bottomPng, middlePng, topPng);
        porytiles::CompilerContext secondaryContext{config, porytiles::CompilerMode::SECONDARY};
        secondaryContext.primaryTileset = &compiledPrimaryTiles;
        compiledTiles = compile(secondaryContext, decompiledTiles);
    }
    else {
        png::image<png::rgba_pixel> bottomPng{config.bottomPrimaryTilesheetPath};
        png::image<png::rgba_pixel> middlePng{config.middlePrimaryTilesheetPath};
        png::image<png::rgba_pixel> topPng{config.topPrimaryTilesheetPath};
        DecompiledTileset decompiledTiles = importLayeredTilesFromPngs(bottomPng, middlePng, topPng);
        porytiles::CompilerContext primaryContext{config, porytiles::CompilerMode::PRIMARY};
        compiledTiles = compile(primaryContext, decompiledTiles);
    }

    std::filesystem::path outputPath(config.outputPath);
    std::filesystem::path palettesDir("palettes");
    std::filesystem::path tilesetFile("tiles.png");
    std::filesystem::path metatilesFile("metatiles.bin");
    std::filesystem::path tilesetPath = config.outputPath / tilesetFile;
    std::filesystem::path metatilesPath = config.outputPath / metatilesFile;
    std::filesystem::path palettesPath = config.outputPath / palettesDir;

    if (std::filesystem::exists(tilesetPath) && !std::filesystem::is_regular_file(tilesetPath)) {
        throw PtException{"`" + tilesetPath.string() + "' exists in output directory but is not a file"};
    }
    if (std::filesystem::exists(palettesPath) && !std::filesystem::is_directory(palettesPath)) {
        throw PtException{"`" + palettesDir.string() + "' exists in output directory but is not a directory"};
    }
    std::filesystem::create_directories(palettesPath);

    emitPalettes(config, compiledTiles, palettesPath);
    emitTilesPng(config, compiledTiles, tilesetPath);

    std::ofstream outMetatiles{metatilesPath.string()};
    emitMetatilesBin(config, outMetatiles, compiledTiles);
    outMetatiles.close();
}

void drive(const Config& config) {
    switch(config.subcommand) {
        case COMPILE_RAW:
            driveCompileRaw(config);
            break;
        case COMPILE:
            driveCompile(config);
            break;
        default:
            throw std::runtime_error{"unknown subcommand setting: " + std::to_string(config.subcommand)};
    }
}

}

TEST_CASE("drive should emit all expected files for simple_metatiles_2 primary set") {
    porytiles::Config config = porytiles::defaultConfig();
    std::filesystem::path parentDir = porytiles::createTmpdir();
    config.secondary = false;
    config.outputPath = parentDir;
    config.subcommand = porytiles::COMPILE;

    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/bottom_primary.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/middle_primary.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/top_primary.png"));
    config.bottomPrimaryTilesheetPath = "res/tests/simple_metatiles_2/bottom_primary.png";
    config.middlePrimaryTilesheetPath = "res/tests/simple_metatiles_2/middle_primary.png";
    config.topPrimaryTilesheetPath = "res/tests/simple_metatiles_2/top_primary.png";

    porytiles::drive(config);

    // TODO : check pal files

    // Check tiles.png

    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/primary_expected_tiles.png"));
    REQUIRE(std::filesystem::exists(parentDir / "tiles.png"));
    png::image<png::index_pixel> expectedPng{"res/tests/simple_metatiles_2/primary_expected_tiles.png"};
    png::image<png::index_pixel> actualPng{parentDir / "tiles.png"};

    std::size_t expectedWidthInTiles = expectedPng.get_width() / porytiles::TILE_SIDE_LENGTH;
    std::size_t expectedHeightInTiles = expectedPng.get_height() / porytiles::TILE_SIDE_LENGTH;
    std::size_t actualWidthInTiles = actualPng.get_width() / porytiles::TILE_SIDE_LENGTH;
    std::size_t actualHeightInTiles = actualPng.get_height() / porytiles::TILE_SIDE_LENGTH;

    CHECK(expectedWidthInTiles == actualWidthInTiles);
    CHECK(expectedHeightInTiles == actualHeightInTiles);

    for (size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
        size_t tileRow = tileIndex / actualWidthInTiles;
        size_t tileCol = tileIndex % actualHeightInTiles;
        for (size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
            size_t pixelRow = (tileRow * porytiles::TILE_SIDE_LENGTH) + (pixelIndex / porytiles::TILE_SIDE_LENGTH);
            size_t pixelCol = (tileCol * porytiles::TILE_SIDE_LENGTH) + (pixelIndex % porytiles::TILE_SIDE_LENGTH);
            CHECK(expectedPng[pixelRow][pixelCol] == actualPng[pixelRow][pixelCol]);
        }
    }


    // Check metatiles.bin

    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/primary_expected_metatiles.bin"));
    REQUIRE(std::filesystem::exists(parentDir / "metatiles.bin"));
    std::FILE* expected;
    std::FILE* actual;

    expected = fopen("res/tests/simple_metatiles_2/primary_expected_metatiles.bin", "r");
    if (expected == NULL) {
        FAIL("std::FILE `expected' was null");
    }
    actual = fopen((parentDir / "metatiles.bin").c_str(), "r");
    if (actual == NULL) {
        fclose(expected);
        FAIL("std::FILE `expected' was null");
    }
    fseek(expected, 0, SEEK_END);
    long expectedSize = ftell(expected);
    rewind(expected);
    fseek(actual, 0, SEEK_END);
    long actualSize = ftell(actual);
    rewind(actual);
    CHECK(expectedSize == actualSize);

    std::uint8_t expectedByte;
    std::uint8_t actualByte;
    std::size_t bytesRead;
    for (long i = 0; i < actualSize; i++) {
        bytesRead = fread(&expectedByte, 1, 1, expected);
        if (bytesRead != 1) {
            FAIL("did not read exactly 1 byte");
        }
        bytesRead = fread(&actualByte, 1, 1, actual);
        if (bytesRead != 1) {
            FAIL("did not read exactly 1 byte");
        }
        CHECK(expectedByte == actualByte);
    }

    fclose(expected);
    fclose(actual);
    std::filesystem::remove_all(parentDir);
}

TEST_CASE("drive should emit all expected files for simple_metatiles_2 secondary set") {
    porytiles::Config config = porytiles::defaultConfig();
    std::filesystem::path parentDir = porytiles::createTmpdir();
    config.secondary = true;
    config.outputPath = parentDir;
    config.subcommand = porytiles::COMPILE;

    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/bottom_primary.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/middle_primary.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/top_primary.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/bottom_secondary.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/middle_secondary.png"));
    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/top_secondary.png"));

    config.bottomPrimaryTilesheetPath = "res/tests/simple_metatiles_2/bottom_primary.png";
    config.middlePrimaryTilesheetPath = "res/tests/simple_metatiles_2/middle_primary.png";
    config.topPrimaryTilesheetPath = "res/tests/simple_metatiles_2/top_primary.png";
    config.bottomSecondaryTilesheetPath = "res/tests/simple_metatiles_2/bottom_secondary.png";
    config.middleSecondaryTilesheetPath = "res/tests/simple_metatiles_2/middle_secondary.png";
    config.topSecondaryTilesheetPath = "res/tests/simple_metatiles_2/top_secondary.png";

    porytiles::drive(config);

    // TODO : check pal files

    // Check tiles.png

    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/secondary_expected_tiles.png"));
    REQUIRE(std::filesystem::exists(parentDir / "tiles.png"));
    png::image<png::index_pixel> expectedPng{"res/tests/simple_metatiles_2/secondary_expected_tiles.png"};
    png::image<png::index_pixel> actualPng{parentDir / "tiles.png"};

    std::size_t expectedWidthInTiles = expectedPng.get_width() / porytiles::TILE_SIDE_LENGTH;
    std::size_t expectedHeightInTiles = expectedPng.get_height() / porytiles::TILE_SIDE_LENGTH;
    std::size_t actualWidthInTiles = actualPng.get_width() / porytiles::TILE_SIDE_LENGTH;
    std::size_t actualHeightInTiles = actualPng.get_height() / porytiles::TILE_SIDE_LENGTH;

    CHECK(expectedWidthInTiles == actualWidthInTiles);
    CHECK(expectedHeightInTiles == actualHeightInTiles);

    for (size_t tileIndex = 0; tileIndex < actualWidthInTiles * actualHeightInTiles; tileIndex++) {
        size_t tileRow = tileIndex / actualWidthInTiles;
        size_t tileCol = tileIndex % actualHeightInTiles;
        for (size_t pixelIndex = 0; pixelIndex < porytiles::TILE_NUM_PIX; pixelIndex++) {
            size_t pixelRow = (tileRow * porytiles::TILE_SIDE_LENGTH) + (pixelIndex / porytiles::TILE_SIDE_LENGTH);
            size_t pixelCol = (tileCol * porytiles::TILE_SIDE_LENGTH) + (pixelIndex % porytiles::TILE_SIDE_LENGTH);
            CHECK(expectedPng[pixelRow][pixelCol] == actualPng[pixelRow][pixelCol]);
        }
    }


    // Check metatiles.bin

    REQUIRE(std::filesystem::exists("res/tests/simple_metatiles_2/secondary_expected_metatiles.bin"));
    REQUIRE(std::filesystem::exists(parentDir / "metatiles.bin"));
    std::FILE* expected;
    std::FILE* actual;

    expected = fopen("res/tests/simple_metatiles_2/secondary_expected_metatiles.bin", "r");
    if (expected == NULL) {
        FAIL("std::FILE `expected' was null");
    }
    actual = fopen((parentDir / "metatiles.bin").c_str(), "r");
    if (actual == NULL) {
        fclose(expected);
        FAIL("std::FILE `expected' was null");
    }
    fseek(expected, 0, SEEK_END);
    long expectedSize = ftell(expected);
    rewind(expected);
    fseek(actual, 0, SEEK_END);
    long actualSize = ftell(actual);
    rewind(actual);
    CHECK(expectedSize == actualSize);

    std::uint8_t expectedByte;
    std::uint8_t actualByte;
    std::size_t bytesRead;
    for (long i = 0; i < actualSize; i++) {
        bytesRead = fread(&expectedByte, 1, 1, expected);
        if (bytesRead != 1) {
            FAIL("did not read exactly 1 byte");
        }
        bytesRead = fread(&actualByte, 1, 1, actual);
        if (bytesRead != 1) {
            FAIL("did not read exactly 1 byte");
        }
        CHECK(expectedByte == actualByte);
    }

    fclose(expected);
    fclose(actual);
    std::filesystem::remove_all(parentDir);
}
