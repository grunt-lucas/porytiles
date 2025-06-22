#pragma once

#include <string>
#include <vector>

#include <porytiles2/domain/entities/rgba_anim.hpp>
#include <porytiles2/domain/entities/rgba_metatile.hpp>

namespace porytiles {

class PorytilesTileset {
    std::string name_;
    std::vector<RgbaMetatile> metatiles_;
    std::vector<RgbaAnim> anims_;
};

} // namespace porytiles
