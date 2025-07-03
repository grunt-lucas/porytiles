#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "porytiles2/domain/model/aggregates/components/PorymapTilesetComponent.hpp"
#include "porytiles2/domain/model/aggregates/components/PorytilesTilesetComponent.hpp"

namespace porytiles {

/**
 * @brief A complete tileset containing both Porytiles and Porymap components.
 */
class Tileset {
public:
  Tileset(std::unique_ptr<PorytilesTilesetComponent> porytiles_component,
          std::unique_ptr<PorymapTilesetComponent> porymap_component)
      : porytiles_component_{std::move(porytiles_component)},
        porymap_component_{std::move(porymap_component)} {}

  const std::string &name() const { return name_; }

  const std::unordered_map<std::string, std::string> &
  artifact_checksums() const {
    return artifact_checksums_;
  }

private:
  std::string name_;
  std::vector<std::string> partner_names_;
  std::unordered_map<std::string, std::string> artifact_checksums_;
  std::unique_ptr<PorytilesTilesetComponent> porytiles_component_;
  std::unique_ptr<PorymapTilesetComponent> porymap_component_;
};

} // namespace porytiles
