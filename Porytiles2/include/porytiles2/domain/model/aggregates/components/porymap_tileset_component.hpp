#pragma once

#include <vector>

#include "porytiles2/domain/model/entities/vram_metatile.hpp"

namespace porytiles2 {

class PorymapTilesetComponent {
  public:
    PorymapTilesetComponent() = default;

    /**
     * @brief Add a metatile to the end of the metatiles vector.
     *
     * @details
     * Moves the provided VramMetatile into the metatiles vector.
     *
     * @param metatile The VramMetatile to move into the vector.
     */
    void push_back(VramMetatile metatile);

    [[nodiscard]] const std::vector<VramMetatile> &metatiles() const {
        return metatiles_;
    }

  private:
    std::vector<VramMetatile> metatiles_;
};

} // namespace porytiles2
