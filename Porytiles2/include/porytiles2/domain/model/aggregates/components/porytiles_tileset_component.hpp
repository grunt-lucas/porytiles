#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/model/entities/rgba_anim.hpp"
#include "porytiles2/domain/model/entities/rgba_metatile.hpp"

namespace porytiles2 {

class PorytilesTilesetComponent {
  std::vector<RgbaMetatile> metatiles_;
  std::vector<RgbaAnim> anims_;
};

} // namespace porytiles2
