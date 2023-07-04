#ifndef PORYTILES_TYPES_H
#define PORYTILES_TYPES_H

#include <vector>
#include <cstdint>
#include <compare>
#include <array>

constexpr std::size_t TILE_ROWS = 8;
constexpr std::size_t TILE_COLS = 8;
constexpr std::size_t TILE_SIZE = TILE_ROWS * TILE_COLS;
constexpr std::size_t PAL_SIZE = 16;

struct Config {
    std::size_t numTilesInPrimary;
    std::size_t numTilesTotal;
    std::size_t numMetatilesInPrimary;
    std::size_t numMetatilesTotal;
    std::size_t numPalettesInPrimary;
    std::size_t numPalettesTotal;
};

struct BGR15 {
    std::uint16_t bgr;

    auto operator<=>(BGR15 const&) const = default;
};

struct RGBA32 {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;

    auto operator<=>(const RGBA32&) const = default;
};

struct RGBATile {
    std::array<RGBA32, TILE_SIZE> rgbaPixels;
};

struct GBATile {
    std::array<std::uint8_t, TILE_SIZE> palIndexes;

    //auto operator<=>(const GBATile&) const = default;
};

struct GBAPalette {
    std::array<BGR15, PAL_SIZE> colors;

    //auto operator<=>(GBAPalette const&) const = default;
};

struct Assignment {
    std::size_t tileIndex;
    std::size_t paletteIndex;
    bool hFlip;
    bool vFlip;
};

struct Compiled {
    std::vector<GBATile> tiles;
    std::vector<GBAPalette> palettes;
    std::vector<Assignment> assignments;
};

struct Decompiled {
    std::vector<RGBATile> tiles;
};

#endif