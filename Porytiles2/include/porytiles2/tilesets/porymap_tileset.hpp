#pragma once

#include <porytiles2/anims/vram_anim.hpp>
#include <porytiles2/metatiles/vram_metatile.hpp>

namespace porytiles {

class PorymapTileset {
    std::vector<VramMetatile> metatiles_;
    std::vector<VramAnim> anims_;
};

} // namespace porytiles
