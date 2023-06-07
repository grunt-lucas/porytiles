#ifndef TSCREATE_TILE_H
#define TSCREATE_TILE_H

#include "rgb_color.h"

#include <png.hpp>
#include <functional>
#include <cstddef>
#include <array>

namespace tscreate {
// Tiles are always 8 by 8 pixels
constexpr int TILE_DIMENSION = 8;

template<typename T>
class Tile {
    std::array<std::array<T, TILE_DIMENSION>, TILE_DIMENSION> pixels;

public:
    explicit Tile(T defaultValue) {
        for (int i = 0; i < TILE_DIMENSION; i++) {
            for (int j = 0; j < TILE_DIMENSION; j++) {
                this->pixels[i][j] = defaultValue;
            }
        }
    }

    Tile(const Tile<T>& other) {
        for (int i = 0; i < TILE_DIMENSION; i++) {
            for (int j = 0; j < TILE_DIMENSION; j++) {
                this->pixels[i][j] = other.pixels[i][j];
            }
        }
    }

    bool operator==(const Tile<T>& other) const {
        for (int i = 0; i < TILE_DIMENSION; i++) {
            for (int j = 0; j < TILE_DIMENSION; j++) {
                if (this->pixels[i][j] != other.pixels[i][j])
                    return false;
            }
        }
        return true;
    }

    T getPixel(long row, long col) const {
        return pixels.at(row).at(col);
    }

    void setPixel(long row, long col, T value) {
        pixels.at(row).at(col) = value;
    }
};

typedef Tile<tscreate::RgbColor> RgbTile;
typedef Tile<int> IndexedTile;
} // namespace tscreate

namespace std {
template<typename T>
struct std::hash<tscreate::Tile<T>> {
    size_t operator()(const tscreate::Tile<T>& tile) const {
        // TODO impl
        return 0;
    }
};
} // namespace tscreate

#endif // TSCREATE_TILE_H