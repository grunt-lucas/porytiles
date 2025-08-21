#pragma once

#include <vector>

#include "porytiles2/domain/model/image.hpp"
#include "porytiles2/domain/model/index_pixel.hpp"
#include "porytiles2/domain/model/rgba_pal.hpp"
#include "porytiles2/domain/model/tilemap_entry.hpp"

namespace porytiles2 {

class PorymapTilesetComponent {
  public:
    PorymapTilesetComponent() : tiles_png_{std::make_unique<Image<IndexPixel>>()} {}

    /**
     * @brief Add a tilemap entry to the end of the entries vector.
     *
     * @details
     * Moves the provided TilemapEntry into the entries vector.
     *
     * @param entry The TilemapEntry to move into the vector.
     */
    void push_back_tilemap_entry(TilemapEntry entry);

    /**
     * @brief Add a palette to the end of the palettes vector.
     *
     * @details
     * Moves the provided RgbaPal into the palettes vector.
     *
     * @param pal The RgbaPal to move into the vector.
     */
    void push_back_pal(RgbaPal pal);

    [[nodiscard]] bool is_empty() const;

    [[nodiscard]] const std::vector<TilemapEntry> &metatiles_bin() const
    {
        return metatiles_bin_;
    }

    [[nodiscard]] const Image<IndexPixel> &tiles_png() const
    {
        return *tiles_png_;
    }

    void tiles_png(std::unique_ptr<Image<IndexPixel>> tiles_png)
    {
        tiles_png_ = std::move(tiles_png);
    }

    [[nodiscard]] const std::vector<RgbaPal> &pals() const
    {
        return pals_;
    }

  private:
    std::vector<TilemapEntry> metatiles_bin_;
    std::unique_ptr<Image<IndexPixel>> tiles_png_;
    std::vector<RgbaPal> pals_;
};

} // namespace porytiles2
