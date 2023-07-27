#include "driver.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <exception>
#include <png.hpp>

#include "doctest.h"
#include "config.h"
#include "emitter.h"
#include "compiler.h"
#include "importer.h"
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
    if (std::filesystem::exists(config.outputPath) && !std::filesystem::is_directory(config.outputPath)) {
        throw PtException{config.outputPath + ": exists but is not a directory"};
    }
    if (!std::filesystem::exists(config.rawTilesheetPath)) {
        throw PtException{config.rawTilesheetPath + ": file does not exist"};
    }
    if (!std::filesystem::is_regular_file(config.rawTilesheetPath)) {
        throw PtException{config.rawTilesheetPath + ": exists but was not a regular file"};
    }

    try {
        // We do this here so if the input is not a PNG, we can catch and give a better error
        png::image<png::rgba_pixel> tilesheetPng{config.rawTilesheetPath};
    }
    catch(const std::exception& exception) {
        throw PtException{config.rawTilesheetPath + " is not a valid PNG file"};
    }

    // confirmed image was a PNG, open it again
    png::image<png::rgba_pixel> tilesheetPng{config.rawTilesheetPath};
    DecompiledTileset decompiledTiles = importRawTilesFromPng(tilesheetPng);
    porytiles::CompilerContext context{config, porytiles::CompilerMode::RAW};
    CompiledTileset compiledTiles = compilePrimary(context, decompiledTiles);

    std::filesystem::path outputPath(config.outputPath);
    std::filesystem::path palettesDir("palettes");
    std::filesystem::path tilesetFile("tiles.png");
    std::filesystem::path tilesetPath = config.outputPath / tilesetFile;
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
}

static void driveCompile(const Config& config) {
    if (std::filesystem::exists(config.outputPath) && !std::filesystem::is_directory(config.outputPath)) {
        throw PtException{config.outputPath + ": exists but is not a directory"};
    }
    if (!std::filesystem::exists(config.bottomTilesheetPath)) {
        throw PtException{config.bottomTilesheetPath + ": file does not exist"};
    }
    if (!std::filesystem::is_regular_file(config.bottomTilesheetPath)) {
        throw PtException{config.bottomTilesheetPath + ": exists but was not a regular file"};
    }
    if (!std::filesystem::exists(config.middleTilesheetPath)) {
        throw PtException{config.middleTilesheetPath + ": file does not exist"};
    }
    if (!std::filesystem::is_regular_file(config.middleTilesheetPath)) {
        throw PtException{config.middleTilesheetPath + ": exists but was not a regular file"};
    }
    if (!std::filesystem::exists(config.topTilesheetPath)) {
        throw PtException{config.topTilesheetPath + ": file does not exist"};
    }
    if (!std::filesystem::is_regular_file(config.topTilesheetPath)) {
        throw PtException{config.topTilesheetPath + ": exists but was not a regular file"};
    }
    if (config.secondary) {
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
    }

    try {
        // We do this here so if the input is not a PNG, we can catch and give a better error
        png::image<png::rgba_pixel> tilesheetPng{config.bottomTilesheetPath};
    }
    catch(const std::exception& exception) {
        throw PtException{config.bottomTilesheetPath + " is not a valid PNG file"};
    }
    try {
        // We do this here so if the input is not a PNG, we can catch and give a better error
        png::image<png::rgba_pixel> tilesheetPng{config.middleTilesheetPath};
    }
    catch(const std::exception& exception) {
        throw PtException{config.middleTilesheetPath + " is not a valid PNG file"};
    }
    try {
        // We do this here so if the input is not a PNG, we can catch and give a better error
        png::image<png::rgba_pixel> tilesheetPng{config.topTilesheetPath};
    }
    catch(const std::exception& exception) {
        throw PtException{config.topTilesheetPath + " is not a valid PNG file"};
    }
    if (config.secondary) {
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
    }

    CompiledTileset compiledTiles{};
    if (config.secondary) {
        png::image<png::rgba_pixel> bottomPrimaryPng{config.bottomPrimaryTilesheetPath};
        png::image<png::rgba_pixel> middlePrimaryPng{config.middlePrimaryTilesheetPath};
        png::image<png::rgba_pixel> topPrimaryPng{config.topPrimaryTilesheetPath};
        DecompiledTileset decompiledPrimaryTiles = importLayeredTilesFromPngs(bottomPrimaryPng, middlePrimaryPng, topPrimaryPng);
        porytiles::CompilerContext primaryContext{config, porytiles::CompilerMode::PRIMARY};
        CompiledTileset compiledPrimaryTiles = compilePrimary(primaryContext, decompiledPrimaryTiles);

        png::image<png::rgba_pixel> bottomPng{config.bottomTilesheetPath};
        png::image<png::rgba_pixel> middlePng{config.middleTilesheetPath};
        png::image<png::rgba_pixel> topPng{config.topTilesheetPath};
        DecompiledTileset decompiledTiles = importLayeredTilesFromPngs(bottomPng, middlePng, topPng);
        porytiles::CompilerContext secondaryContext{config, porytiles::CompilerMode::SECONDARY};
        compiledTiles = compileSecondary(secondaryContext, decompiledTiles, compiledPrimaryTiles);
    }
    else {
        png::image<png::rgba_pixel> bottomPng{config.bottomTilesheetPath};
        png::image<png::rgba_pixel> middlePng{config.middleTilesheetPath};
        png::image<png::rgba_pixel> topPng{config.topTilesheetPath};
        DecompiledTileset decompiledTiles = importLayeredTilesFromPngs(bottomPng, middlePng, topPng);
        porytiles::CompilerContext primaryContext{config, porytiles::CompilerMode::PRIMARY};
        compiledTiles = compilePrimary(primaryContext, decompiledTiles);
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

}

TEST_CASE("drive should emit all expected files for simple_metatiles_2 secondary set") {

}
