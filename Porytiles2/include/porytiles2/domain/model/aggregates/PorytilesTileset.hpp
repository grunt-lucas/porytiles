#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/model/entities/RgbaAnim.hpp"
#include "porytiles2/domain/model/entities/RgbaMetatile.hpp"

namespace porytiles {

class PorytilesTileset {
  std::string name_;
  std::vector<RgbaMetatile> metatiles_;
  std::vector<RgbaAnim> anims_;
  std::vector<std::string> partner_tileset_names_;
};

} // namespace porytiles
