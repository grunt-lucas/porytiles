#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

#include "porytiles/domain/models/animation.hpp"
#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/tilemap_entry.hpp"

namespace porytiles {

class PorymapTilesetComponent {
  public:
    PorymapTilesetComponent();

    /// @brief Add a TilemapEntry to the end of the entry vector.
    ///
    /// @details
    /// Moves the provided TilemapEntry into the entry vector.
    ///
    /// @param entry The TilemapEntry to move into the vector.
    void push_back_tilemap_entry(TilemapEntry entry);

    /// @brief Add a MetatileAttribute to the end of the attribute vector.
    ///
    /// @details
    /// Moves the provided MetatileAttribute into the attribute vector.
    ///
    /// @param attribute The MetatileAttribute to move into the vector.
    void push_back_attribute(MetatileAttribute attribute);

    void set_pal(std::size_t pal_index, Palette<Rgba32, pal::max_size> pal);

    [[nodiscard]] const Palette<Rgba32, pal::max_size> &pal_at(std::size_t pal_index) const;

    [[nodiscard]] bool is_empty() const;

    [[nodiscard]] ChainableResult<LayerMode> detect_layer_mode() const;

    void add_anim(Animation<IndexPixel> anim);

    [[nodiscard]] bool has_anim(const std::string &name) const
    {
        return anims_.contains(name);
    }

    [[nodiscard]] const Animation<IndexPixel> &anim_for_name(const std::string &name) const
    {
        return anims_.at(name);
    }

    [[nodiscard]] const std::vector<TilemapEntry> &metatiles_bin() const
    {
        return metatiles_bin_;
    }

    [[nodiscard]] std::vector<TilemapEntry> &metatiles_bin()
    {
        return metatiles_bin_;
    }

    void metatiles_bin(const std::vector<TilemapEntry> &new_entries)
    {
        metatiles_bin_ = new_entries;
    }

    [[nodiscard]] const std::vector<MetatileAttribute> &metatile_attributes_bin() const
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

    [[nodiscard]] const std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> &pals() const
    {
        return pals_;
    }

    [[nodiscard]] const std::map<std::string, Animation<IndexPixel>> &anims() const
    {
        return anims_;
    }

    [[nodiscard]] std::map<std::string, Animation<IndexPixel>> &anims()
    {
        return anims_;
    }

  private:
    std::vector<TilemapEntry> metatiles_bin_;
    std::vector<MetatileAttribute> metatile_attributes_;
    Image<IndexPixel> tiles_png_;
    std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> pals_;
    std::map<std::string, Animation<IndexPixel>> anims_;
};

} // namespace porytiles
