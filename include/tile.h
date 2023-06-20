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

    void setPixel(size_t row, size_t col, const T& value) {
        if (row >= TILE_DIMENSION)
            throw std::out_of_range{
                    "internal: Tile::setPixel row argument out of bounds (" + std::to_string(row) + ")"};
        if (col >= TILE_DIMENSION)
            throw std::out_of_range{
                    "internal: Tile::setPixel col argument out of bounds (" + std::to_string(col) + ")"};
        pixels.at(row * TILE_DIMENSION + col) = value;
    }

    void setPixel(size_t index, const T& value) {
        if (index >= PIXEL_COUNT)
            throw std::out_of_range{
                    "internal: Tile::setPixel index argument out of bounds (" + std::to_string(index) + ")"};
        pixels.at(index) = value;
    }

    [[nodiscard]] bool isUniformly(const T& value) const {
        for (size_t i = 0; i < PIXEL_COUNT; i++) {
            if (pixels[i] != value) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] std::unordered_set<T> uniquePixels(const T& transparencyColor) const {
        std::unordered_set<T> uniquePixels;
        for (size_t i = 0; i < PIXEL_COUNT; i++) {
            if (pixels[i] != transparencyColor) {
                uniquePixels.insert(pixels[i]);
            }
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

doctest::String toString(const std::unordered_set<RgbColor>& colorSet);

doctest::String toString(const std::unordered_set<png::byte>& indexSet);

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

#endif // PORYTILES_TILE_H