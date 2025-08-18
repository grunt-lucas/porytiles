#pragma once

#include <vector>

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/index_pixel.hpp"
#include "porytiles2/domain/model/rgba_pal.hpp"
#include "porytiles2/domain/model/tilemap_entry.hpp"

namespace porytiles2 {

class PorymapTilesetComponent {
  public:
    PorymapTilesetComponent() = default;

    /**
     * @brief Add a tilemap entry to the end of the entries vector.
     *
     * @details
     * Moves the provided TilemapEntry into the entries vector.
     *
     * @param entry The TilemapEntry to move into the vector.
     */
    void push_back(TilemapEntry entry);

    [[nodiscard]] bool is_empty() const;

    [[nodiscard]] const std::vector<TilemapEntry> &metatiles_bin() const {
        return metatiles_bin_;
    }

    [[nodiscard]] const Image<IndexPixel> &tiles_png() const {
        return tiles_png_;
    }

    [[nodiscard]] const std::vector<RgbaPal> &pals() const {
        return pals_;
    }

  private:
    std::vector<TilemapEntry> metatiles_bin_;
    Image<IndexPixel> tiles_png_;
    std::vector<RgbaPal> pals_;
};

} // namespace porytiles2
