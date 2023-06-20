#ifndef PORYTILES_TILE_H
#define PORYTILES_TILE_H

#include "rgb_color.h"
#include "palette.h"
#include "cli_parser.h"

#include <doctest.h>
#include <png.hpp>
#include <functional>
#include <cstddef>
#include <array>
#include <stdexcept>
#include <unordered_set>
#include <algorithm>

namespace porytiles {
// Tiles are always 8 by 8 pixels
constexpr size_t TILE_DIMENSION = 8;
constexpr size_t PIXEL_COUNT = TILE_DIMENSION * TILE_DIMENSION;

template<typename T>
class Tile {
    // TODO make this a vector for better move semantics
    std::array<T, PIXEL_COUNT> pixels;

public:
    Tile();

    explicit Tile(T defaultValue) {
        for (size_t i = 0; i < PIXEL_COUNT; i++) {
            pixels[i] = defaultValue;
        }
    }

    Tile(const Tile<T>& other) {
        for (size_t i = 0; i < PIXEL_COUNT; i++) {
            pixels[i] = other.pixels[i];
        }
    }

    // TODO we defined a copy ctor so we should follow Rule of 5: if we switch class to use vector, we can just use defaults

    bool operator==(const Tile<T>& other) const {
        for (size_t i = 0; i < PIXEL_COUNT; i++) {
            if (pixels[i] != other.pixels[i])
                return false;
        }
        return true;
    }

    bool operator!=(const Tile<T>& other) const {
        return !(*this == other);
    }

    [[nodiscard]] T getPixel(size_t row, size_t col) const {
        if (row >= TILE_DIMENSION)
            throw std::out_of_range{
                    "internal: Tile::getPixel row argument out of bounds (" + std::to_string(row) + ")"};
        if (col >= TILE_DIMENSION)
            throw std::out_of_range{
                    "internal: Tile::getPixel col argument out of bounds (" + std::to_string(col) + ")"};
        return pixels.at(row * TILE_DIMENSION + col);
    }

    [[nodiscard]] T getPixel(size_t index) const {
        if (index >= PIXEL_COUNT)
            throw std::out_of_range{
                    "internal: Tile::getPixel index argument out of bounds (" + std::to_string(index) + ")"};
        return pixels.at(index);
    }

    [[nodiscard]] const std::array<T, PIXEL_COUNT>& getPixels() const {
        return pixels;
    }

    void setPixel(size_t row, size_t col, T value) {
        if (row >= TILE_DIMENSION)
            throw std::out_of_range{
                    "internal: Tile::setPixel row argument out of bounds (" + std::to_string(row) + ")"};
        if (col >= TILE_DIMENSION)
            throw std::out_of_range{
                    "internal: Tile::setPixel col argument out of bounds (" + std::to_string(col) + ")"};
        pixels.at(row * TILE_DIMENSION + col) = value;
    }

    void setPixel(size_t index, T value) {
        if (index >= PIXEL_COUNT)
            throw std::out_of_range{
                    "internal: Tile::setPixel index argument out of bounds (" + std::to_string(index) + ")"};
        pixels.at(index) = value;
    }

    [[nodiscard]] bool isUniformly(T value) const {
        for (size_t i = 0; i < PIXEL_COUNT; i++) {
            if (pixels[i] != value)
                return false;
        }
        return true;
    }

    [[nodiscard]] std::unordered_set<T> uniquePixels(T transparencyColor) const {
        std::unordered_set<T> uniquePixels;
        for (size_t i = 0; i < PIXEL_COUNT; i++) {
            if (pixels[i] != transparencyColor)
                uniquePixels.insert(pixels[i]);
        }
        return uniquePixels;
    }

    [[nodiscard]] Tile<T> getHorizontalFlip() const {
        Tile<T> flippedTile;
        for (size_t row = 0; row < TILE_DIMENSION; row++) {
            for (size_t col = 0; col < TILE_DIMENSION / 2; col++) {
                size_t colInverted = TILE_DIMENSION - 1 - col;
                flippedTile.setPixel(row, col, this->getPixel(row, colInverted));
                flippedTile.setPixel(row, colInverted, this->getPixel(row, col));
            }
        }
        return flippedTile;
    }

    [[nodiscard]] Tile<T> getVerticalFlip() const {
        Tile<T> flippedTile;
        for (size_t col = 0; col < TILE_DIMENSION; col++) {
            for (size_t row = 0; row < TILE_DIMENSION / 2; row++) {
                size_t rowInverted = TILE_DIMENSION - 1 - row;
                flippedTile.setPixel(row, col, this->getPixel(rowInverted, col));
                flippedTile.setPixel(rowInverted, col, this->getPixel(row, col));
            }
        }
        return flippedTile;
    }

    [[nodiscard]] Tile<T> getDiagonalFlip() const {
        Tile<T> flippedTile;
        flippedTile = this->getHorizontalFlip().getVerticalFlip();
        return flippedTile;
    }

    [[nodiscard]] std::unordered_set<T> pixelsNotInPalette(const Palette& palette) const;
};

typedef Tile<RgbColor> RgbTile;
typedef Tile<png::byte> IndexedTile;

doctest::String toString(const RgbTile& tile);

doctest::String toString(const IndexedTile& tile);

} // namespace porytiles

namespace std {
template<typename T>
struct hash<porytiles::Tile<T>> {
    size_t operator()(const porytiles::Tile<T>& tile) const {
        size_t result = 0;
        for (size_t row = 0; row < porytiles::TILE_DIMENSION; row++) {
            for (size_t col = 0; col < porytiles::TILE_DIMENSION; col++) {
                result = result * 31 + std::hash<T>()(tile.getPixel(row, col));
            }
        }
        return result;
    }
};
} // namespace std

/*
 * Test Cases
 */
TEST_CASE("Logically equivalent Tile<T> objects should be equivalent under op==") {
    SUBCASE("Logically equivalent RgbTiles should match") {
        porytiles::RgbTile tile1{{0, 0, 0}};
        porytiles::RgbTile tile2{{0, 0, 0}};
        CHECK(tile1 == tile2);
    }
    SUBCASE("Logically divergent RgbTiles should not match") {
        porytiles::RgbTile tile1{{0, 0, 0}};
        porytiles::RgbTile tile2{{1, 1, 1}};
        CHECK(tile1 != tile2);
    }
    SUBCASE("Logically equivalent IndexedTiles should match") {
        porytiles::IndexedTile tile1{0};
        porytiles::IndexedTile tile2{0};
        CHECK(tile1 == tile2);
    }
    SUBCASE("Logically divergent IndexedTiles should not match") {
        porytiles::IndexedTile tile1{0};
        porytiles::IndexedTile tile2{1};
        CHECK(tile1 != tile2);
    }
}

TEST_CASE("Tile<T> copy ctor should create a copy that is equivalent under op==") {
    SUBCASE("It should work for RgbTile") {
        porytiles::RgbTile tile1{{0, 0, 0}};
        porytiles::RgbTile tile2{tile1};
        CHECK(tile1 == tile2);
    }
    SUBCASE("It should work for IndexedTile") {
        porytiles::IndexedTile tile1{12};
        porytiles::IndexedTile tile2{tile1};
        CHECK(tile1 == tile2);
    }
}

TEST_CASE("Tile<T> getPixel should either return the requested pixel or throw on out-of-bounds") {
    porytiles::RgbTile zeroRgbTile{{0, 0, 0}};

    SUBCASE("It should return {0,0,0} for pixel row=0,col=0") {
        CHECK(zeroRgbTile.getPixel(0, 0) == porytiles::RgbColor{0, 0, 0});
    }
    SUBCASE("It should return {0,0,0} for pixel index=10") {
        CHECK(zeroRgbTile.getPixel(10) == porytiles::RgbColor{0, 0, 0});
    }
    SUBCASE("It should throw a std::out_of_range for out-of-bounds pixel index=1000") {
        porytiles::RgbColor dummy;
        CHECK_THROWS_WITH_AS(dummy = zeroRgbTile.getPixel(1000),
                             "internal: Tile::getPixel index argument out of bounds (1000)", const std::out_of_range&);
    }
    SUBCASE("It should throw a std::out_of_range for out-of-bounds pixel row=1000,col=0") {
        porytiles::RgbColor dummy;
        CHECK_THROWS_WITH_AS(dummy = zeroRgbTile.getPixel(1000, 0),
                             "internal: Tile::getPixel row argument out of bounds (1000)", const std::out_of_range&);
    }
    SUBCASE("It should throw a std::out_of_range for out-of-bounds pixel row=0,col=1000") {
        porytiles::RgbColor dummy;
        CHECK_THROWS_WITH_AS(dummy = zeroRgbTile.getPixel(0, 1000),
                             "internal: Tile::getPixel col argument out of bounds (1000)", const std::out_of_range&);
    }
}

TEST_CASE("Tile<T> setPixel should either return the requested pixel or throw on out-of-bounds") {
    porytiles::IndexedTile zeroIndexedTile{0};

    SUBCASE("It should set pixel row=1,col=0 to value 1") {
        zeroIndexedTile.setPixel(1, 0, 1);
        CHECK(zeroIndexedTile.getPixel(1, 0) == 1);
    }
    SUBCASE("It should set pixel index=22 to value 12") {
        zeroIndexedTile.setPixel(22, 12);
        CHECK(zeroIndexedTile.getPixel(22) == 12);
    }
    SUBCASE("It should throw a std::out_of_range for out-of-bounds pixel index=1000") {
        CHECK_THROWS_WITH_AS(zeroIndexedTile.setPixel(1000, 0),
                             "internal: Tile::setPixel index argument out of bounds (1000)", const std::out_of_range&);
    }
    SUBCASE("It should throw a std::out_of_range for out-of-bounds pixel row=1000,col=0") {
        CHECK_THROWS_WITH_AS(zeroIndexedTile.setPixel(1000, 0, 0),
                             "internal: Tile::setPixel row argument out of bounds (1000)", const std::out_of_range&);
    }
    SUBCASE("It should throw a std::out_of_range for out-of-bounds pixel row=0,col=1000") {
        CHECK_THROWS_WITH_AS(zeroIndexedTile.setPixel(0, 1000, 0),
                             "internal: Tile::setPixel col argument out of bounds (1000)", const std::out_of_range&);
    }
}

#endif // PORYTILES_TILE_H