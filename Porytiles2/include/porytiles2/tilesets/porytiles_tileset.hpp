#pragma once

#include <vector>

#include <porytiles2/anims/rgba_anim.hpp>
#include <porytiles2/metatiles/rgba_metatile.hpp>

namespace porytiles {

class PorytilesTileset {
    std::vector<RgbaMetatile> metatiles_;
    std::vector<RgbaAnim> anims_;
};

} // namespace porytiles
