#ifndef PORYTILES_TYPES_H
#define PORYTILES_TYPES_H

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <compare>
#include <array>
#include <vector>
#include <string>

#include "doctest.h"

/**
 * TODO : fill in doc comment for this header
 */

namespace porytiles {

// --------------------
// |    DATA TYPES    |
// --------------------

/**
 * BGR15 color format. 5 bits per color with blue in most significant bits. Top bit unused.
 */
struct BGR15 {
    std::uint16_t bgr;

    auto operator<=>(const BGR15&) const = default;

    friend std::ostream& operator<<(std::ostream&, const BGR15&);
};

extern const BGR15 BGR_BLACK;
extern const BGR15 BGR_RED;
extern const BGR15 BGR_GREEN;
extern const BGR15 BGR_BLUE;
extern const BGR15 BGR_YELLOW;
extern const BGR15 BGR_MAGENTA;
extern const BGR15 BGR_CYAN;
extern const BGR15 BGR_WHITE;
extern const BGR15 BGR_GREY;

/**
 * RGBA32 format. 1 byte per color and 1 byte for alpha channel.
 */
struct RGBA32 {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::uint8_t alpha;

    std::string jasc() const {
        return std::to_string(red) + " " + std::to_string(green) + " " + std::to_string(blue);
    }

    auto operator<=>(const RGBA32&) const = default;

    friend std::ostream& operator<<(std::ostream&, const RGBA32&);
};

constexpr std::uint8_t ALPHA_TRANSPARENT = 0;
constexpr std::uint8_t ALPHA_OPAQUE = 255;

extern const RGBA32 RGBA_BLACK;
extern const RGBA32 RGBA_RED;
extern const RGBA32 RGBA_GREEN;
extern const RGBA32 RGBA_BLUE;
extern const RGBA32 RGBA_YELLOW;
extern const RGBA32 RGBA_MAGENTA;
extern const RGBA32 RGBA_CYAN;
extern const RGBA32 RGBA_WHITE;
extern const RGBA32 RGBA_GREY;

/**
 * A tile of RGBA32 colors.
 */
constexpr std::size_t TILE_SIDE_LENGTH = 8;
constexpr std::size_t TILE_NUM_PIX = TILE_SIDE_LENGTH * TILE_SIDE_LENGTH;
struct RGBATile {
    std::array<RGBA32, TILE_NUM_PIX> pixels;

    [[nodiscard]] RGBA32 getPixel(size_t row, size_t col) const {
        if (row >= TILE_SIDE_LENGTH) {
            throw std::out_of_range{
                    "internal: RGBATile::getPixel row argument out of bounds (" + std::to_string(row) + ")"};
        }
        if (col >= TILE_SIDE_LENGTH) {
            throw std::out_of_range{
                    "internal: RGBATile::getPixel col argument out of bounds (" + std::to_string(col) + ")"};
        }
        return pixels[row * TILE_SIDE_LENGTH + col];
    }

#if defined(__GNUG__) && !defined(__clang__)

    auto operator<=>(const RGBATile&) const = default;

#else
    // TODO : manually implement for clang, default spaceship for std::array not yet supported by libc++
    // https://discourse.llvm.org/t/c-spaceship-operator-default-marked-as-deleted-with-std-array-member/66529/5
    // https://reviews.llvm.org/D132265
    // https://reviews.llvm.org/rG254986d2df8d8407b46329e452c16748d29ed4cd
    // auto operator<=>(const RGBATile&) const = default;
    auto operator<=>(const RGBATile& other) const {
        if (this->pixels == other.pixels) {
            return std::strong_ordering::equal;
        }
        else if (this->pixels < other.pixels) {
            return std::strong_ordering::less;
        }
        return std::strong_ordering::greater;
    }

    auto operator==(const RGBATile& other) const {
        return this->pixels == other.pixels;
    }
#endif

    friend std::ostream& operator<<(std::ostream&, const RGBATile&);
};

extern const RGBATile RGBA_TILE_BLACK;
extern const RGBATile RGBA_TILE_RED;
extern const RGBATile RGBA_TILE_GREEN;
extern const RGBATile RGBA_TILE_BLUE;
extern const RGBATile RGBA_TILE_YELLOW;
extern const RGBATile RGBA_TILE_MAGENTA;
extern const RGBATile RGBA_TILE_CYAN;
extern const RGBATile RGBA_TILE_WHITE;

/**
 * A tile of palette indexes.
 */
struct GBATile {
    std::array<std::uint8_t, TILE_NUM_PIX> paletteIndexes;

#if defined(__GNUG__) && !defined(__clang__)

    auto operator<=>(const GBATile&) const = default;

#else
    // TODO : manually implement for clang, default spaceship for std::array not yet supported by libc++
    // https://discourse.llvm.org/t/c-spaceship-operator-default-marked-as-deleted-with-std-array-member/66529/5
    // https://reviews.llvm.org/D132265
    // https://reviews.llvm.org/rG254986d2df8d8407b46329e452c16748d29ed4cd
    auto operator<=>(const GBATile&) const = default;
    // auto operator<=>(const GBATile& other) const {
    //     if (this->paletteIndexes == other.paletteIndexes) {
    //         return std::strong_ordering::equal;
    //     }
    //     else if (this->paletteIndexes < other.paletteIndexes) {
    //         return std::strong_ordering::less;
    //     }
    //     return std::strong_ordering::greater;
    // }

    // auto operator==(const GBATile& other) const {
    //     return this->paletteIndexes == other.paletteIndexes;
    // }
#endif
};

/**
 * A palette of PAL_SIZE (16) BGR15 colors.
 */
constexpr std::size_t PAL_SIZE = 16;
constexpr std::size_t MAX_BG_PALETTES = 16;
struct GBAPalette {
    std::array<BGR15, PAL_SIZE> colors;

#if defined(__GNUG__) && !defined(__clang__)

    auto operator<=>(const GBAPalette&) const = default;

#else
    // TODO : manually implement for clang, default spaceship for std::array not yet supported by libc++
    // https://discourse.llvm.org/t/c-spaceship-operator-default-marked-as-deleted-with-std-array-member/66529/5
    // https://reviews.llvm.org/D132265
    // https://reviews.llvm.org/rG254986d2df8d8407b46329e452c16748d29ed4cd
    // auto operator<=>(const GBAPalette&) const = default;
    auto operator<=>(const GBAPalette& other) const {
        if (this->colors == other.colors) {
            return std::strong_ordering::equal;
        }
        else if (this->colors < other.colors) {
            return std::strong_ordering::less;
        }
        return std::strong_ordering::greater;
    }

    auto operator==(const GBAPalette& other) const {
        return this->colors == other.colors;
    }
#endif
};

/**
 * A tile assignment, i.e. the representation of a tile within a metatile. Maps a given tile index to a hardware palette
 * index and the corresponding flips.
 */
struct Assignment {
    std::size_t tileIndex;
    std::size_t paletteIndex;
    bool hFlip;
    bool vFlip;
};

/**
 * A compiled tileset.
 *
 * The `tiles' field contains the normalized tiles from the input tilesheets. This field can be directly written out to
 * `tiles.png'.
 *
 * The `palettes' field are hardware palettes, i.e. there should be numPalsInPrimary palettes for a primary tileset, or
 * `numPalettesTotal - numPalsInPrimary' palettes for a secondary tileset.
 *
 * The `assignments' vector contains the actual tile palette assignments and flips which can be used to construct the
 * final metatiles.
 */
struct CompiledTileset {
    std::vector<GBATile> tiles;
    std::vector<GBAPalette> palettes;
    std::vector<Assignment> assignments;
};

/**
 * A decompiled tileset, which is just a vector of RGBATiles.
 */
struct DecompiledTileset {
    std::vector<RGBATile> tiles;
};

/*
 * Normalized types
 */

/**
 * TODO : fill in doc comment
 */
struct NormalizedPixels {
    std::array<std::uint8_t, TILE_NUM_PIX> paletteIndexes;

#if defined(__GNUG__) && !defined(__clang__)

    auto operator<=>(const NormalizedPixels&) const = default;

#else
    // TODO : manually implement for clang, default spaceship for std::array not yet supported by libc++
    // https://discourse.llvm.org/t/c-spaceship-operator-default-marked-as-deleted-with-std-array-member/66529/5
    // https://reviews.llvm.org/D132265
    // https://reviews.llvm.org/rG254986d2df8d8407b46329e452c16748d29ed4cd
    // auto operator<=>(const NormalizedPixels&) const = default;
    auto operator<=>(const NormalizedPixels& other) const {
        if (this->paletteIndexes == other.paletteIndexes) {
            return std::strong_ordering::equal;
        }
        else if (this->paletteIndexes < other.paletteIndexes) {
            return std::strong_ordering::less;
        }
        return std::strong_ordering::greater;
    }

    auto operator==(const NormalizedPixels& other) const {
        return this->paletteIndexes == other.paletteIndexes;
    }
#endif
};

/**
 * TODO : fill in doc comment
 */
struct NormalizedPalette {
    int size;
    std::array<BGR15, PAL_SIZE> colors;

#if defined(__GNUG__) && !defined(__clang__)

    auto operator<=>(const NormalizedPalette&) const = default;

#else
    // TODO : manually implement for clang, default spaceship for std::array not yet supported by libc++
    // https://discourse.llvm.org/t/c-spaceship-operator-default-marked-as-deleted-with-std-array-member/66529/5
    // https://reviews.llvm.org/D132265
    // https://reviews.llvm.org/rG254986d2df8d8407b46329e452c16748d29ed4cd
    // auto operator<=>(const NormalizedPalette&) const = default;
    auto operator<=>(const NormalizedPalette& other) const {
        if (this->colors == other.colors) {
            return std::strong_ordering::equal;
        }
        else if (this->colors < other.colors) {
            return std::strong_ordering::less;
        }
        return std::strong_ordering::greater;
    }

    auto operator==(const NormalizedPalette& other) const {
        return this->colors == other.colors;
    }
#endif
};

/**
 * TODO : fill in doc comment
 */
struct NormalizedTile {
    // TODO : can we collapse these types into NormalizedTile?
    NormalizedPixels pixels;
    NormalizedPalette palette;
    bool hFlip;
    bool vFlip;

    bool transparent() const { return palette.size == 1; }

#if defined(__GNUG__) && !defined(__clang__)
    // auto operator<=>(const NormalizedTile&) const = default;
#else
    // TODO : manually implement for clang, default spaceship for std::array not yet supported by libc++
    // https://discourse.llvm.org/t/c-spaceship-operator-default-marked-as-deleted-with-std-array-member/66529/5
    // https://reviews.llvm.org/D132265
    // https://reviews.llvm.org/rG254986d2df8d8407b46329e452c16748d29ed4cd
    // auto operator<=>(const NormalizedTile&) const = default;
#endif

    void setPixel(size_t row, size_t col, uint8_t value) {
        if (row >= TILE_SIDE_LENGTH) {
            throw std::out_of_range{
                    "internal: NormalizedTile::setPixel row argument out of bounds (" + std::to_string(row) + ")"};
        }
        if (col >= TILE_SIDE_LENGTH) {
            throw std::out_of_range{
                    "internal: NormalizedTile::setPixel col argument out of bounds (" + std::to_string(col) + ")"};
        }
        pixels.paletteIndexes[row * TILE_SIDE_LENGTH + col] = value;
    }
};


// -------------------
// |    FUNCTIONS    |
// -------------------

BGR15 rgbaToBgr(const RGBA32& rgba);

RGBA32 bgrToRgba(const BGR15& bgr);

}


// -------------------------
// |    HASH FUNCTIONS     |
// -------------------------

namespace std {

template<>
struct hash<porytiles::BGR15> {
    size_t operator()(const porytiles::BGR15& bgr) const noexcept {
        return hash<uint16_t>{}(bgr.bgr);
    }
};

template<>
struct hash<porytiles::GBATile> {
    size_t operator()(const porytiles::GBATile& tile) const noexcept {
        // TODO : better hash function.
        size_t hashValue = 0;
        for (auto index: tile.paletteIndexes) {
            hashValue ^= hash<uint8_t>{}(index);
        }
        return hashValue;
    }
};

template<>
struct hash<porytiles::NormalizedPixels> {
    size_t operator()(const porytiles::NormalizedPixels& pixels) const noexcept {
        // TODO : better hash function.
        size_t hashValue = 0;
        for (auto pixel: pixels.paletteIndexes) {
            hashValue ^= hash<uint8_t>{}(pixel);
        }
        return hashValue;
    }
};

template<>
struct hash<porytiles::NormalizedPalette> {
    size_t operator()(const porytiles::NormalizedPalette& palette) const noexcept {
        // TODO : better hash function.
        size_t hashValue = 0;
        hashValue ^= hash<int>{}(palette.size);
        for (auto color: palette.colors) {
            hashValue ^= hash<porytiles::BGR15>{}(color);
        }
        return hashValue;
    }
};

template<>
struct hash<porytiles::NormalizedTile> {
    size_t operator()(const porytiles::NormalizedTile& tile) const noexcept {
        // TODO : better hash function.
        size_t hashValue = 0;
        hashValue ^= hash<porytiles::NormalizedPixels>{}(tile.pixels);
        hashValue ^= hash<porytiles::NormalizedPalette>{}(tile.palette);
        hashValue ^= hash<bool>{}(tile.hFlip);
        hashValue ^= hash<bool>{}(tile.vFlip);
        return hashValue;
    }
};

}

#endif // PORYTILES_TYPES_H
