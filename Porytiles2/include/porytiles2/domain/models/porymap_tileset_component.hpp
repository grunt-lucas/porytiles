#pragma once

#include <array>
#include <vector>

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/rgba_pal.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"

namespace porytiles2 {

class PorymapTilesetComponent {
  public:
    PorymapTilesetComponent() = default;

    /**
     * @brief Add a TilemapEntry to the end of the entry vector.
     *
     * @details
     * Moves the provided TilemapEntry into the entry vector.
     *
     * @param entry The TilemapEntry to move into the vector.
     */
    void push_back_tilemap_entry(TilemapEntry entry);

    /**
     * @brief Add a MetatileAttribute to the end of the attribute vector.
     *
     * @details
     * Moves the provided MetatileAttribute into the attribute vector.
     *
     * @param attribute The MetatileAttribute to move into the vector.
     */
    void push_back_attribute(MetatileAttribute attribute);

    void set_pal(RgbaPal pal, int pal_index);

    [[nodiscard]] const RgbaPal &pal_at(int pal_index) const;

    [[nodiscard]] bool is_empty() const;

    [[nodiscard]] const std::vector<TilemapEntry> &metatiles_bin() const
    {
        return metatiles_bin_;
    }

    [[nodiscard]] const std::vector<MetatileAttribute> &metatile_attributes() const
    {
        return metatile_attributes_;
    }

    [[nodiscard]] const Image<IndexPixel> &tiles_png() const
    {
        return tiles_png_;
    }

    void tiles_png(const Image<IndexPixel> &tiles_png)
    {
        tiles_png_ = tiles_png;
    }

    [[nodiscard]] const std::array<RgbaPal, pal::num_pals> &pals() const
    {
        return pals_;
    }

  private:
    std::vector<TilemapEntry> metatiles_bin_;
    std::vector<MetatileAttribute> metatile_attributes_;
    Image<IndexPixel> tiles_png_;
    std::array<RgbaPal, pal::num_pals> pals_;
};

} // namespace porytiles2
