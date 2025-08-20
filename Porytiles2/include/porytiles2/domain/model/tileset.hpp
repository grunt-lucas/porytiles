#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "porytiles2/domain/model/porymap_tileset_component.hpp"
#include "porytiles2/domain/model/porytiles_tileset_component.hpp"

namespace porytiles2 {

/**
 * @brief A complete tileset containing both Porytiles and Porymap components.
 */
class Tileset {
  public:
    Tileset(
        std::string name,
        std::unique_ptr<PorytilesTilesetComponent> porytiles_component,
        std::unique_ptr<PorymapTilesetComponent> porymap_component)
        : name_{std::move(name)}, porytiles_component_{std::move(porytiles_component)},
          porymap_component_{std::move(porymap_component)} {

        if (porytiles_component_ == nullptr) {
            panic("porytiles_component was null");
        }
        if (porymap_component_ == nullptr) {
            panic("porymap_component was null");
        }
    }

    [[nodiscard]] const std::string &name() const {
        return name_;
    }

    [[nodiscard]] const PorytilesTilesetComponent *porytiles_component() const {
        return porytiles_component_.get();
    }

    [[nodiscard]] const PorymapTilesetComponent *porymap_component() const {
        return porymap_component_.get();
    }

    void porytiles_component(std::unique_ptr<PorytilesTilesetComponent> porytiles_component) {
        if (porytiles_component == nullptr) {
            panic("porytiles_component was null");
        }
        porytiles_component_ = std::move(porytiles_component);
    }

    void porymap_component(std::unique_ptr<PorymapTilesetComponent> porymap_component) {
        if (porymap_component == nullptr) {
            panic("porymap_component was null");
        }
        porymap_component_ = std::move(porymap_component);
    }

  private:
    std::string name_;
    std::unique_ptr<PorytilesTilesetComponent> porytiles_component_;
    std::unique_ptr<PorymapTilesetComponent> porymap_component_;
};

} // namespace porytiles2
