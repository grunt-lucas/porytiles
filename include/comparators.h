#ifndef COMPARATORS_H
#define COMPARATORS_H

#include <png.hpp>

namespace tscreate {
class rgb_pixel_eq {
public:
    bool operator()(const png::rgb_pixel& p1, const png::rgb_pixel& p2) const;
};

class rgb_pixel_hasher {
public:
    std::size_t operator()(const png::rgb_pixel& p) const;
};
}

#endif // COMPARATORS_H