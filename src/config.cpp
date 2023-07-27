#include "config.h"

namespace porytiles {

Config defaultConfig() {
    Config config;

    // Fieldmap params
    setPokeemeraldDefaultTilesetParams(config);
    config.numTilesPerMetatile = 12;

    // Input params
    config.transparencyColor = RGBA_MAGENTA;
    config.maxRecurseCount = 2000000;
    config.secondary = false;

    // Output params
    config.tilesPngPaletteMode = GREYSCALE;

    // Command params
    config.subcommand = COMPILE_RAW;
    config.verbose = false;

    return config;
}

void setPokeemeraldDefaultTilesetParams(Config& config) {
    config.numTilesInPrimary = 512;
    config.numTilesTotal = 1024;
    config.numMetatilesInPrimary = 512;
    config.numMetatilesTotal = 1024;
    config.numPalettesInPrimary = 6;
    config.numPalettesTotal = 13;
}

void setPokefireredDefaultTilesetParams(Config& config) {
    config.numTilesInPrimary = 640;
    config.numTilesTotal = 1024;
    config.numMetatilesInPrimary = 640;
    config.numMetatilesTotal = 1024;
    config.numPalettesInPrimary = 7;
    config.numPalettesTotal = 13;
}

void setPokerubyDefaultTilesetParams(Config& config) {
    config.numTilesInPrimary = 512;
    config.numTilesTotal = 1024;
    config.numMetatilesInPrimary = 512;
    config.numMetatilesTotal = 1024;
    config.numPalettesInPrimary = 6;
    config.numPalettesTotal = 12;
}

}