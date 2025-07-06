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

  [[nodiscard]] const std::string &name() const { return name_; }

  [[nodiscard]] const PorytilesTilesetComponent &porytiles_component() const {
    return *porytiles_component_;
  }

  [[nodiscard]] const PorymapTilesetComponent &porymap_component() const {
    return *porymap_component_;
  }

  void porytiles_component(std::unique_ptr<PorytilesTilesetComponent> porytiles_component) {
    porytiles_component_ = std::move(porytiles_component);
  }

  void porymap_component(std::unique_ptr<PorymapTilesetComponent> porymap_component) {
    porymap_component_ = std::move(porymap_component);
  }

private:
  std::string name_;
  std::vector<std::string> partner_names_;
  std::unique_ptr<PorytilesTilesetComponent> porytiles_component_;
  std::unique_ptr<PorymapTilesetComponent> porymap_component_;
};

} // namespace porytiles
