#pragma once

#include <vector>

#include "../anims/rgba_anim.hpp"
#include "../metatiles/rgba_metatile.hpp"

namespace porytiles {

class PorytilesTileset {
    std::vector<RgbaMetatile> metatiles_;
    std::vector<RgbaAnim> anims_;
};

} // namespace porytiles
