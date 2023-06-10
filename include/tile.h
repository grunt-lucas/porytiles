#ifndef TSCREATE_TILE_H
#define TSCREATE_TILE_H

#include "rgb_color.h"
#include "palette.h"

#include <png.hpp>
#include <functional>
#include <cstddef>
#include <array>
#include <stdexcept>
#include <unordered_set>
#include <algorithm>

namespace tscreate {
// Tiles are always 8 by 8 pixels
constexpr int TILE_DIMENSION = 8;
constexpr int PIXEL_COUNT = TILE_DIMENSION * TILE_DIMENSION;

template<typename T>
class Tile {
    std::array<T, PIXEL_COUNT> pixels;

public:
    explicit Tile(T defaultValue) {
        for (int i = 0; i < PIXEL_COUNT; i++) {
            pixels[i] = defaultValue;
        }
    }

    Tile(const Tile<T>& other) {
        for (int i = 0; i < PIXEL_COUNT; i++) {
            pixels[i] = other.pixels[i];
        }
    }

    bool operator==(const Tile<T>& other) const {
        for (int i = 0; i < PIXEL_COUNT; i++) {
            if (pixels[i] != other.pixels[i])
                return false;
        }
        return true;
    }

    [[nodiscard]] T getPixel(long row, long col) const {
        if (row >= TILE_DIMENSION)
            throw std::runtime_error{
                    "internal: Tile::getPixel row argument out of bounds (" + std::to_string(row) + ")"};
        if (col >= TILE_DIMENSION)
            throw std::runtime_error{
                    "internal: Tile::getPixel col argument out of bounds (" + std::to_string(col) + ")"};
        return pixels.at(row * TILE_DIMENSION + col);
    }

    void setPixel(long row, long col, T value) {
        if (row >= TILE_DIMENSION)
            throw std::runtime_error{
                    "internal: Tile::setPixel row argument out of bounds (" + std::to_string(row) + ")"};
        if (col >= TILE_DIMENSION)
            throw std::runtime_error{
                    "internal: Tile::setPixel col argument out of bounds (" + std::to_string(col) + ")"};
        pixels.at(row * TILE_DIMENSION + col) = value;
    }

    [[nodiscard]] bool isUniformly(T value) const {
        for (int i = 0; i < PIXEL_COUNT; i++) {
            if (pixels[i] != value)
                return false;
        }
        return true;
    }

    [[nodiscard]] std::unordered_set<T> uniquePixels(T transparencyColor) const {
        std::unordered_set<T> uniquePixels;
        for (int i = 0; i < PIXEL_COUNT; i++) {
            if (pixels[i] != transparencyColor)
                uniquePixels.insert(pixels[i]);
        }
        return uniquePixels;
    }
};

typedef Tile<RgbColor> RgbTile;
typedef Tile<png::byte> IndexedTile;
} // namespace tscreate

namespace std {
template<typename T>
struct std::hash<tscreate::Tile<T>> {
    size_t operator()(const tscreate::Tile<T>& tile) const {
        size_t result = 0;
        for (long row = 0; row < tscreate::TILE_DIMENSION; row++) {
            for (long col = 0; col < tscreate::TILE_DIMENSION; col++) {
                result = result * 31 + std::hash(tile.getPixel(row, col));
            }
        }
        return result;
    }
};
} // namespace tscreate

#endif // TSCREATE_TILE_H