#pragma once

#include <vector>

#include <porytiles2/domain/anims/rgba_anim.hpp>
#include <porytiles2/domain/metatiles/rgba_metatile.hpp>

namespace porytiles {

class PorytilesTileset {
    std::vector<RgbaMetatile> metatiles_;
    std::vector<RgbaAnim> anims_;
};

} // namespace porytiles
