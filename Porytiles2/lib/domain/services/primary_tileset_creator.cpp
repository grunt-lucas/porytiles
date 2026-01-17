#include "porytiles2/domain/services/primary_tileset_creator.hpp"

#include <cstddef>
#include <memory>

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/porytiles_tileset_component.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace porytiles2 {

namespace {

// Sample tileset colors - natural GBA-style palette
constexpr Rgba32 grass_bulk{112, 192, 160};
constexpr Rgba32 grass_light{160, 224, 192};
constexpr Rgba32 grass_dark{56, 192, 128};

constexpr std::array<std::array<Rgba32, metatile::side_length_pix>, metatile::side_length_pix> grass_layer{
    // Row 0
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 1
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 2
    std::array{
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 3
    std::array{
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 4
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 5
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 6
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 7
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 8
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 9
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 10
    std::array{
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_dark,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 11
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_dark,
        grass_bulk,
        grass_bulk,
        grass_dark,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 12
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_dark,
        grass_bulk,
        grass_dark,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 13
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_dark,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 14
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 15
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk}};

void set_grass_at(Image<Rgba32> &img, std::size_t metatile_index)
{
    const std::size_t col_offset = metatile_index * metatile::side_length_pix;
    for (std::size_t row = 0; row < metatile::side_length_pix; ++row) {
        for (std::size_t col = 0; col < metatile::side_length_pix; ++col) {
            img.set(row, col_offset + col, grass_layer[row][col]);
        }
    }
}

void set_flower_at(Image<Rgba32> &img, std::size_t metatile_index)
{
    // TODO: hardcode the white flower here once we implement anim compilation
    const std::size_t col_offset = metatile_index * metatile::side_length_pix;
    for (std::size_t row = 0; row < metatile::side_length_pix; ++row) {
        for (std::size_t col = 0; col < metatile::side_length_pix; ++col) {
            img.set(row, col_offset + col, grass_layer[row][col]);
        }
    }
}

} // anonymous namespace

ChainableResult<std::unique_ptr<PorytilesTilesetComponent>>
PrimaryTilesetCreator::create_sample_porytiles_component(const std::string &tileset_name) const
{
    PT_UNWRAP_TILESET_CONFIG_PTR(
        config_, extrinsic_transparency, tileset_name, std::unique_ptr<PorytilesTilesetComponent>);

    auto component = std::make_unique<PorytilesTilesetComponent>();

    Image<Rgba32> bottom{128, 16, extrinsic_transparency};
    Image<Rgba32> middle{128, 16, extrinsic_transparency};
    Image<Rgba32> top{128, 16, extrinsic_transparency};

    set_grass_at(middle, 1);
    set_grass_at(bottom, 2);
    set_flower_at(middle, 2);

    component->bottom(bottom);
    component->middle(middle);
    component->top(top);

    // Set metatile attributes for the sample tiles
    const auto behavior_result = behavior_map_->lookup("MB_TALL_GRASS");
    if (behavior_result.has_value()) {
        // Only set the behavior if the user project had an MB_TALL_GRASS behavior
        component->insert_attribute(1, MetatileAttribute{LayerType::normal, behavior_result.value()});
    }

    // TODO: add animation params for white flower. This needs to wait until after we implement animation compilation.

    return component;
}

} // namespace porytiles2
