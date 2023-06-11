#ifndef PORYTILES_TILE_H
#define PORYTILES_TILE_H

#include "rgb_color.h"
#include "palette.h"
#include "cli_parser.h"

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
    std::array<T, PIXEL_COUNT> pixels;

public:
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

    virtual ~Tile() = default;

    bool operator==(const Tile<T>& other) const {
        for (size_t i = 0; i < PIXEL_COUNT; i++) {
            if (pixels[i] != other.pixels[i])
                return false;
        }
        return true;
    }

    [[nodiscard]] T getPixel(size_t row, size_t col) const {
        if (row >= TILE_DIMENSION)
            throw std::runtime_error{
                    "internal: Tile::getPixel row argument out of bounds (" + std::to_string(row) + ")"};
        if (col >= TILE_DIMENSION)
            throw std::runtime_error{
                    "internal: Tile::getPixel col argument out of bounds (" + std::to_string(col) + ")"};
        return pixels.at(row * TILE_DIMENSION + col);
    }

    void setPixel(size_t row, size_t col, T value) {
        if (row >= TILE_DIMENSION)
            throw std::runtime_error{
                    "internal: Tile::setPixel row argument out of bounds (" + std::to_string(row) + ")"};
        if (col >= TILE_DIMENSION)
            throw std::runtime_error{
                    "internal: Tile::setPixel col argument out of bounds (" + std::to_string(col) + ")"};
        pixels.at(row * TILE_DIMENSION + col) = value;
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

    [[nodiscard]] virtual bool isControlTile() const;

    [[nodiscard]] virtual std::unordered_set<T> pixelsNotInPalette(const Palette& palette) const;
};

typedef Tile<RgbColor> RgbTile;

template<>
[[nodiscard]] bool RgbTile::isControlTile() const;

template<>
[[nodiscard]] std::unordered_set<RgbColor> RgbTile::pixelsNotInPalette(const Palette& palette) const;

typedef Tile<png::byte> IndexedTile;

template<>
[[nodiscard]] bool IndexedTile::isControlTile() const;

template<>
[[nodiscard]] std::unordered_set<png::byte> IndexedTile::pixelsNotInPalette(const Palette& palette) const;
} // namespace porytiles

namespace std {
template<typename T>
struct hash<porytiles::Tile<T>> {
    size_t operator()(const porytiles::Tile<T>& tile) const {
        size_t result = 0;
        for (size_t row = 0; row < porytiles::TILE_DIMENSION; row++) {
            for (size_t col = 0; col < porytiles::TILE_DIMENSION; col++) {
                result = result * 31 + std::hash(tile.getPixel(row, col));
            }
        }
        return result;
    }
};
} // namespace std

#endif // PORYTILES_TILE_H