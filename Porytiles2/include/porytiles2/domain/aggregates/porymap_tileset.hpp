#pragma once

#include <string>
#include <vector>

#include <porytiles2/domain/entities/vram_anim.hpp>
#include <porytiles2/domain/entities/vram_metatile.hpp>

namespace porytiles {

class PorymapTileset {
    std::string name_;
    std::vector<VramMetatile> metatiles_;
    std::vector<VramAnim> anims_;
};

} // namespace porytiles
