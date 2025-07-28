#include "porytiles2/domain/model/rgba_metatile.hpp"

#include <utility>

#include "fmt/format.h"

#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

const RgbaTile &RgbaMetatile::bottom(const std::size_t i) const {
    if (i > 3) {
        panic(fmt::format("index {} out of bounds: must be 0-3", i));
    }
    return bottom_[i];
}

void RgbaMetatile::set_bottom(const std::size_t i, RgbaTile tile) {
    if (i > 3) {
        panic(fmt::format("index {} out of bounds: must be 0-3", i));
    }
    bottom_[i] = std::move(tile);
}

const RgbaTile &RgbaMetatile::middle(const std::size_t i) const {
    if (i > 3) {
        panic(fmt::format("index {} out of bounds: must be 0-3", i));
    }
    return middle_[i];
}

void RgbaMetatile::set_middle(const std::size_t i, RgbaTile tile) {
    if (i > 3) {
        panic(fmt::format("index {} out of bounds: must be 0-3", i));
    }
    middle_[i] = std::move(tile);
}

const RgbaTile &RgbaMetatile::top(const std::size_t i) const {
    if (i > 3) {
        panic(fmt::format("index {} out of bounds: must be 0-3", i));
    }
    return top_[i];
}

void RgbaMetatile::set_top(const std::size_t i, RgbaTile tile) {
    if (i > 3) {
        panic(fmt::format("index {} out of bounds: must be 0-3", i));
    }
    top_[i] = std::move(tile);
}

} // namespace porytiles2