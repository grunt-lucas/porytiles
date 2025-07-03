#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/model/entities/VramAnim.hpp"
#include "porytiles2/domain/model/entities/VramMetatile.hpp"

namespace porytiles {

class PorymapTilesetComponent {
  std::vector<VramMetatile> metatiles_;
  std::vector<VramAnim> anims_;
};

} // namespace porytiles
