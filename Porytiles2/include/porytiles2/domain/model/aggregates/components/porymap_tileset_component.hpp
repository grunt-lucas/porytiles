#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/model/entities/vram_anim.hpp"
#include "porytiles2/domain/model/entities/vram_metatile.hpp"

namespace porytiles2 {

class PorymapTilesetComponent {
    std::vector<VramMetatile> metatiles_;
    std::vector<VramAnim> anims_;
};

} // namespace porytiles2
