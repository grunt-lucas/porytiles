#pragma once

#include <string>
#include <vector>

#include "porytiles2/domain/model/entities/RgbaMetatile.hpp"

namespace porytiles {

class PorytilesLayout {
  std::string name_;
  std::vector<RgbaMetatile> map_;
  std::vector<RgbaMetatile> border_;
  std::string primary_tileset_name_;
  std::string secondary_tileset_name_;
};

} // namespace porytiles
