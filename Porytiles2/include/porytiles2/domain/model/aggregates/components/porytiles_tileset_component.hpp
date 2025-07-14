#pragma once

#include <vector>

#include "porytiles2/domain/model/entities/rgba_metatile.hpp"

namespace porytiles2 {

class PorytilesTilesetComponent {
  public:
    PorytilesTilesetComponent() = default;

    /**
     * @brief Add a metatile to the end of the metatiles vector.
     *
     * @details
     * Moves the provided RgbaMetatile into the metatiles vector.
     *
     * @param metatile The RgbaMetatile to move into the vector.
     */
    void push_back(RgbaMetatile metatile);

    [[nodiscard]] const std::vector<RgbaMetatile> &metatiles() const {
        return metatiles_;
    }

  private:
    std::vector<RgbaMetatile> metatiles_;
};

} // namespace porytiles2
