#pragma once

#include <array>
#include <vector>

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/index_pixel.hpp"
#include "porytiles2/domain/model/rgba_pal.hpp"
#include "tilemap_entry.hpp"

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
    void push_back_tilemap_entry(TilemapEntry entry);

    void set_pal(RgbaPal pal, int pal_index);

    [[nodiscard]] const RgbaPal &pal_at(int pal_index) const;

    [[nodiscard]] bool is_empty() const;

    [[nodiscard]] const std::vector<TilemapEntry> &metatiles_bin() const
    {
        return metatiles_bin_;
    }

    [[nodiscard]] const Image<IndexPixel> &tiles_png() const
    {
        return tiles_png_;
    }

    void tiles_png(const Image<IndexPixel> &tiles_png)
    {
        tiles_png_ = tiles_png;
    }

    [[nodiscard]] const std::array<RgbaPal, 16> &pals() const
    {
        return pals_;
    }

  private:
    std::vector<TilemapEntry> metatiles_bin_;
    Image<IndexPixel> tiles_png_;
    // TODO: don't hardcode 16 here
    std::array<RgbaPal, 16> pals_;
};

} // namespace porytiles2
