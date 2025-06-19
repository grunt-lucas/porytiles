#pragma once

#include <porytiles2/domain/anims/vram_anim.hpp>
#include <porytiles2/domain/metatiles/vram_metatile.hpp>

namespace porytiles {

class PorymapTileset {
    std::vector<VramMetatile> metatiles_;
    std::vector<VramAnim> anims_;
};

} // namespace porytiles
