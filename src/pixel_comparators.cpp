#include "pixel_comparators.h"

#include <png.hpp>
#include <cstddef>
#include <functional>

namespace tscreate {
bool rgb_pixel_eq::operator()(const png::rgb_pixel& p1, const png::rgb_pixel& p2) const {
    return p1.red == p2.red && p1.green == p2.green && p1.blue == p2.blue;
}

std::size_t rgb_pixel_hasher::operator()(const png::rgb_pixel& p) const {
    using std::size_t;
    using std::hash;

    return ((hash<int>()(p.red)
             ^ (hash<int>()(p.green) << 1)) >> 1)
             ^ (hash<int>()(p.blue) << 1);
}
}
