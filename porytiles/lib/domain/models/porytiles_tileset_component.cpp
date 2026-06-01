#include "porytiles/domain/models/porytiles_tileset_component.hpp"

#include <format>

#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

void PorytilesTilesetComponent::insert_attribute(std::size_t metatile_id, MetatileAttribute attribute)
{
    if (metatile_attributes_.contains(metatile_id)) {
        panic("id " + std::to_string(metatile_id) + " already exists in attr map");
    }
    metatile_attributes_.insert({metatile_id, std::move(attribute)});
}

std::optional<MetatileAttribute> PorytilesTilesetComponent::get_attribute(std::size_t metatile_id) const
{
    if (metatile_attributes_.contains(metatile_id)) {
        return metatile_attributes_.at(metatile_id);
    }
    return std::nullopt;
}

void PorytilesTilesetComponent::set_pal(std::size_t pal_index, Palette<Rgba32, pal::max_size> pal)
{
    if (pal_index >= pal::num_pals) {
        panic(std::format("invalid pal index {}: out of range", pal_index));
    }
    pals_[pal_index] = std::move(pal);
}

const std::optional<Palette<Rgba32, pal::max_size>> &PorytilesTilesetComponent::pal_at(std::size_t pal_index) const
{
    if (pal_index >= pal::num_pals) {
        panic(std::format("invalid pal index {}: out of range", pal_index));
    }
    return pals_[pal_index];
}

bool PorytilesTilesetComponent::is_empty() const
{
    return bottom_.size() == 0 && middle_.size() == 0 && top_.size() == 0;
}

ChainableResult<LayerMode> PorytilesTilesetComponent::detect_layer_mode(const Rgba32 &extrinsic) const
{
    // Pre-check: all three images must have identical dimensions
    if (bottom_.width() != middle_.width() || bottom_.width() != top_.width() || bottom_.height() != middle_.height() ||
        bottom_.height() != top_.height()) {
        panic(
            std::format(
                "layer images have mismatched dimensions: bottom={}x{}, middle={}x{}, top={}x{}",
                bottom_.width(),
                bottom_.height(),
                middle_.width(),
                middle_.height(),
                top_.width(),
                top_.height()));
    }

    const std::size_t width = bottom_.width();
    const std::size_t height = bottom_.height();

    // Iterate over each 8x8 region
    for (std::size_t region_row = 0; region_row < height; region_row += tile::side_length_pix) {
        for (std::size_t region_col = 0; region_col < width; region_col += tile::side_length_pix) {
            bool bottom_has_opaque = false;
            bool middle_has_opaque = false;
            bool top_has_opaque = false;

            // Check each pixel in the current 8x8 region
            for (std::size_t row = region_row; row < region_row + tile::side_length_pix && row < height; ++row) {
                for (std::size_t col = region_col; col < region_col + tile::side_length_pix && col < width; ++col) {
                    if (!bottom_.at(row, col).is_transparent(extrinsic)) {
                        bottom_has_opaque = true;
                    }
                    if (!middle_.at(row, col).is_transparent(extrinsic)) {
                        middle_has_opaque = true;
                    }
                    if (!top_.at(row, col).is_transparent(extrinsic)) {
                        top_has_opaque = true;
                    }

                    // Early exit if all three layers have opaque pixels in this region
                    if (bottom_has_opaque && middle_has_opaque && top_has_opaque) {
                        return LayerMode::triple;
                    }
                }
            }
        }
    }

    // If no region had opaque pixels on all three layers, it's dual mode
    return LayerMode::dual;
}

void PorytilesTilesetComponent::add_anim(Animation<Rgba32> anim)
{
    const std::string &name = anim.name();
    if (anims_.contains(name)) {
        panic("animation '" + name + "' already exists in PorytilesTilesetComponent");
    }
    anims_.insert({name, std::move(anim)});
}

} // namespace porytiles
