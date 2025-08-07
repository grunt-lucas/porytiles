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
    Tileset() = default;

    Tileset(std::unique_ptr<PorytilesTilesetComponent> porytiles_component,
            std::unique_ptr<PorymapTilesetComponent> porymap_component)
        : porytiles_component_{std::move(porytiles_component)}, porymap_component_{std::move(porymap_component)} {}

    [[nodiscard]] const std::string &name() const {
        return name_;
    }

    void name(std::string name) {
        name_ = std::move(name);
    }

    [[nodiscard]] const std::vector<std::string> &partner_names() const {
        return partner_names_;
    }

    void partner_names(std::vector<std::string> partner_names) {
        partner_names_ = std::move(partner_names);
    }

    [[nodiscard]] const PorytilesTilesetComponent *porytiles_component() const {
        return porytiles_component_.get();
    }

    [[nodiscard]] const PorymapTilesetComponent *porymap_component() const {
        return porymap_component_.get();
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

} // namespace porytiles2
