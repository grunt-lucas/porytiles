#ifndef TSCREATE_TILE_H
#define TSCREATE_TILE_H

#include "rgb_color.h"

#include <png.hpp>
#include <functional>
#include <cstddef>

namespace tscreate {
extern const int TILE_DIMENSION;

template<typename T>
class Tile {
    T pixels[8][8];

public:
    explicit Tile(T defaultValue) {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                this->pixels[i][j] = defaultValue;
            }
        }
    }

    Tile(const Tile<T>& other) {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                this->pixels[i][j] = other.pixels[i][j];
            }
        }
    }

    bool operator==(const Tile<T>& other) const {
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                if (this->pixels[i][j] != other.pixels[i][j])
                    return false;
            }
        }
        return true;
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