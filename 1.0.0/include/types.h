#ifndef PORYTILES_TYPES_H
#define PORYTILES_TYPES_H

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <compare>
#include <array>
#include <vector>

#include "doctest.h"
#include "constants.h"

namespace porytiles {

// --------------------
// |    DATA TYPES    |
// --------------------

/*
 * BGR15 color format. 5 bits per color with blue in most significant bits. Top bit unused.
 */
struct BGR15 {
    std::uint16_t bgr;

    auto operator<=>(const BGR15&) const = default;
};

std::ostream& operator<<(std::ostream&, const BGR15&);

/*
 * RGBA32 format. 1 byte per color and 1 byte for alpha channel.
 */
struct RGBA32 {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;

    auto operator<=>(const RGBA32&) const = default;
};

std::ostream& operator<<(std::ostream&, const RGBA32&);

/*
 * A tile of RGBA32 colors.
 */
struct RGBATile {
    std::array<RGBA32, TILE_SIZE> pixels;
};

/*
 * A tile of palette indexes.
 */
struct GBATile {
    std::array<std::uint8_t, TILE_SIZE> paletteIndexes;

#if defined(__GNUG__) && !defined(__clang__)
    auto operator<=>(GBATile const&) const = default;
#else
    // TODO : manually implement for clang, not yet supported by libc++
    // https://discourse.llvm.org/t/c-spaceship-operator-default-marked-as-deleted-with-std-array-member/66529/5
#endif
};

/*
 * A palette of PAL_SIZE (16) BGR15 colors.
 */
struct GBAPalette {
    std::array<BGR15, PAL_SIZE> colors;

#if defined(__GNUG__) && !defined(__clang__)
    auto operator<=>(GBAPalette const&) const = default;
#else
    // TODO : manually implement for clang, not yet supported by libc++
    // https://discourse.llvm.org/t/c-spaceship-operator-default-marked-as-deleted-with-std-array-member/66529/5
#endif
};

/*
 * A tile assignment, i.e. the representation of a tile within a metatile. Maps a given tile index to a hardware palette
 * index and the corresponding flips.
 */
struct Assignment {
    std::size_t tileIndex;
    std::size_t paletteIndex;
    bool hFlip;
    bool vFlip;
};

/*
 * A compiled tileset. The tiles here are normalized. The palettes here are hardware palettes, i.e. there should be
 * numPalsInPrimary palettes for a primary tileset, or numPalettesTotal - numPalsInPrimary palettes for a secondary
 * tileset. The assignments vector contains the actual tile palette assignments and flips which can be used to construct
 * the final metatiles.
 */
struct Compiled {
    std::vector<GBATile> tiles;
    std::vector<GBAPalette> palettes;
    std::vector<Assignment> assignments;
};

/*
 * A decompiled tileset, which is just a vector of RGBATiles.
 */
struct Decompiled {
    std::vector<RGBATile> tiles;
};


// --------------------
// |    FUNCTIONS     |
// --------------------

BGR15 rgbaToBGR(const RGBA32& rgba);
}


// -------------------------
// |    HASH FUNCTIONS     |
// -------------------------

namespace std {
template<>
struct hash<porytiles::BGR15> {
    size_t operator()(porytiles::BGR15 const& bgr) const noexcept {
        return hash<uint16_t>{}(bgr.bgr);
    }
};
}

#endif // PORYTILES_TYPES_H
