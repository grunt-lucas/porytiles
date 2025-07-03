#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/model/entities/VramAnim.hpp"
#include "porytiles2/domain/model/entities/VramMetatile.hpp"

namespace porytiles {

class PorymapTileset {
  std::string name_;
  std::vector<VramMetatile> metatiles_;
  std::vector<VramAnim> anims_;
  std::vector<std::string> partner_tileset_names_;
};

} // namespace porytiles
