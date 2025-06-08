#pragma once

#include "../anims/vram_anim.hpp"
#include "../metatiles/vram_metatile.hpp"

namespace porytiles {

class PorymapTileset {
    std::vector<VramMetatile> metatiles_;
    std::vector<VramAnim> anims_;
};

} // namespace porytiles
