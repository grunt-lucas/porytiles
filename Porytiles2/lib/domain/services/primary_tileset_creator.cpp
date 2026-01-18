#include "porytiles2/domain/services/primary_tileset_creator.hpp"

#include <cstddef>
#include <memory>
#include <vector>

#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/animation_frame.hpp"
#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/porytiles_tileset_component.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace porytiles2 {

namespace {

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

constexpr Rgba32 flower_petal{255, 197, 148};
constexpr Rgba32 flower_core{205, 65, 82};
constexpr Rgba32 flower_shadow{24, 164, 106};
constexpr Rgba32 flower_outline_light{57, 139, 49};
constexpr Rgba32 flower_outline_dark{57, 82, 0};
constexpr Rgba32 flower_leaf{131, 197, 98};

constexpr std::array<std::array<Rgba32, metatile::side_length_pix>, metatile::side_length_pix> flower_frame_key{
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
        grass_bulk},

    // Row 2
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        flower_outline_light,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_outline_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 3
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 4
    std::array{
        grass_light,
        grass_bulk,
        flower_outline_light,
        flower_outline_dark,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_light,
        flower_outline_light,
        flower_outline_light,
        grass_bulk},

    // Row 5
    std::array{
        grass_bulk,
        flower_outline_light,
        flower_leaf,
        flower_leaf,
        flower_leaf,
        flower_outline_dark,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_light,
        flower_leaf,
        flower_leaf,
        flower_leaf,
        flower_outline_light},

    // Row 6
    std::array{
        grass_bulk,
        flower_outline_light,
        flower_leaf,
        flower_leaf,
        flower_outline_light,
        flower_outline_light,
        flower_outline_light,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_outline_light,
        flower_leaf,
        flower_leaf,
        flower_outline_dark},

    // Row 7
    std::array{
        grass_bulk,
        grass_bulk,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_light,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_shadow},

    // Row 8
    std::array{
        flower_shadow,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_shadow},

    // Row 9
    std::array{
        flower_shadow,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        grass_bulk},

    // Row 10
    std::array{
        grass_bulk,
        flower_shadow,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_light,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        grass_bulk},

    // Row 11
    std::array{
        grass_bulk,
        flower_outline_light,
        flower_outline_light,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_light,
        flower_outline_dark,
        flower_shadow,
        flower_leaf,
        flower_outline_dark,
        flower_outline_light,
        flower_outline_light,
        flower_outline_light,
        flower_leaf,
        flower_outline_light,
        grass_bulk},

    // Row 12
    std::array{
        grass_bulk,
        flower_outline_dark,
        flower_leaf,
        flower_leaf,
        flower_outline_light,
        flower_outline_dark,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_outline_dark,
        flower_outline_light,
        flower_leaf,
        flower_leaf,
        flower_outline_dark,
        grass_bulk},

    // Row 13
    std::array{
        flower_shadow,
        flower_outline_dark,
        flower_leaf,
        flower_outline_light,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_leaf,
        flower_outline_light,
        flower_outline_dark,
        grass_bulk},

    // Row 14
    std::array{
        grass_bulk,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        grass_bulk},

    // Row 15
    std::array{
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_shadow,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_shadow,
        grass_bulk,
        grass_bulk}};

// Frame 0 matches key frame in this case
constexpr std::array<std::array<Rgba32, metatile::side_length_pix>, metatile::side_length_pix> flower_frame_0 =
    flower_frame_key;

constexpr std::array<std::array<Rgba32, metatile::side_length_pix>, metatile::side_length_pix> flower_frame_1{
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
        grass_bulk},

    // Row 2
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        flower_outline_light,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_outline_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 3
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_light,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 4
    std::array{
        grass_light,
        grass_bulk,
        flower_outline_light,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_light,
        flower_outline_light,
        grass_bulk},

    // Row 5
    std::array{
        grass_bulk,
        flower_outline_light,
        flower_leaf,
        flower_leaf,
        flower_leaf,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_dark,
        flower_leaf,
        flower_leaf,
        flower_leaf,
        flower_outline_light},

    // Row 6
    std::array{
        grass_bulk,
        flower_outline_light,
        flower_leaf,
        flower_leaf,
        flower_outline_light,
        flower_outline_light,
        flower_outline_light,
        flower_outline_light,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_leaf,
        flower_leaf,
        flower_outline_light},

    // Row 7
    std::array{
        grass_bulk,
        flower_shadow,
        flower_outline_light,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_light,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_dark},

    // Row 8
    std::array{
        grass_bulk,
        flower_shadow,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_dark},

    // Row 9
    std::array{
        grass_bulk,
        flower_shadow,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow},

    // Row 10
    std::array{
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_outline_dark,
        flower_outline_light,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        grass_bulk},

    // Row 11
    std::array{
        grass_bulk,
        grass_bulk,
        flower_outline_dark,
        flower_leaf,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_leaf,
        flower_outline_dark,
        flower_outline_light,
        flower_outline_light,
        flower_outline_light,
        flower_leaf,
        flower_outline_light,
        grass_bulk},

    // Row 12
    std::array{
        grass_bulk,
        flower_outline_light,
        flower_leaf,
        flower_leaf,
        flower_outline_light,
        flower_outline_dark,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_outline_dark,
        flower_outline_light,
        flower_leaf,
        flower_leaf,
        flower_outline_dark,
        grass_bulk},

    // Row 13
    std::array{
        flower_shadow,
        flower_outline_dark,
        flower_leaf,
        flower_outline_light,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_leaf,
        flower_outline_light,
        flower_outline_dark,
        grass_bulk},

    // Row 14
    std::array{
        grass_bulk,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        grass_bulk},

    // Row 15
    std::array{
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_shadow,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_shadow,
        grass_bulk,
        grass_bulk}};

constexpr std::array<std::array<Rgba32, metatile::side_length_pix>, metatile::side_length_pix> flower_frame_2{
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
        grass_bulk},

    // Row 2
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        flower_outline_light,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_outline_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 3
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 4
    std::array{
        grass_light,
        grass_bulk,
        flower_outline_light,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        grass_bulk,
        flower_outline_light,
        flower_outline_light,
        flower_outline_light,
        grass_bulk},

    // Row 5
    std::array{
        grass_bulk,
        flower_outline_light,
        flower_leaf,
        flower_leaf,
        flower_outline_dark,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_light,
        flower_outline_dark,
        flower_leaf,
        flower_leaf,
        flower_leaf,
        flower_outline_light},

    // Row 6
    std::array{
        grass_bulk,
        flower_outline_light,
        flower_leaf,
        flower_leaf,
        flower_outline_light,
        flower_outline_light,
        flower_outline_light,
        flower_outline_dark,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_outline_light,
        flower_outline_light,
        flower_outline_light,
        flower_leaf,
        flower_outline_dark},

    // Row 7
    std::array{
        flower_outline_light,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_light,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_light,
        flower_outline_dark,
        flower_shadow},

    // Row 8
    std::array{
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow},

    // Row 9
    std::array{
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        grass_bulk},

    // Row 10
    std::array{
        flower_shadow,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_light,
        flower_outline_dark,
        flower_shadow,
        grass_bulk},

    // Row 11
    std::array{
        grass_bulk,
        flower_outline_dark,
        flower_outline_light,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_light,
        flower_outline_dark,
        flower_shadow,
        flower_leaf,
        flower_outline_dark,
        flower_outline_light,
        flower_outline_light,
        flower_outline_light,
        flower_leaf,
        flower_outline_light,
        grass_bulk},

    // Row 12
    std::array{
        grass_bulk,
        flower_outline_dark,
        flower_leaf,
        flower_leaf,
        flower_outline_light,
        flower_outline_dark,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_outline_dark,
        flower_outline_light,
        flower_leaf,
        flower_leaf,
        flower_outline_dark,
        grass_bulk},

    // Row 13
    std::array{
        flower_shadow,
        flower_outline_dark,
        flower_leaf,
        flower_outline_light,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_leaf,
        flower_outline_light,
        flower_outline_dark,
        grass_bulk},

    // Row 14
    std::array{
        grass_bulk,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        grass_bulk},

    // Row 15
    std::array{
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_shadow,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_shadow,
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

void set_flower_key_frame_at(Image<Rgba32> &img, std::size_t metatile_index)
{
    // TODO: hardcode the flower here once we implement anim compilation
    const std::size_t col_offset = metatile_index * metatile::side_length_pix;
    for (std::size_t row = 0; row < metatile::side_length_pix; ++row) {
        for (std::size_t col = 0; col < metatile::side_length_pix; ++col) {
            img.set(row, col_offset + col, flower_frame_key[row][col]);
        }
    }
}

/**
 * @brief Converts a 16x16 metatile pixel array to a vector of 4 PixelTiles.
 *
 * @details
 * Extracts tiles in row-major order (top-left, top-right, bottom-left, bottom-right).
 *
 * @param metatile_pixels The 16x16 pixel array representing a metatile
 * @return A vector of 4 PixelTiles extracted from the metatile
 */
[[nodiscard]] std::vector<PixelTile<Rgba32>> metatile_array_to_tiles(
    const std::array<std::array<Rgba32, metatile::side_length_pix>, metatile::side_length_pix> &metatile_pixels)
{
    std::vector<PixelTile<Rgba32>> tiles;
    tiles.reserve(4);

    // Extract 4 tiles: 2 rows x 2 cols
    for (std::size_t tile_row = 0; tile_row < 2; ++tile_row) {
        for (std::size_t tile_col = 0; tile_col < 2; ++tile_col) {
            std::array<Rgba32, tile::size_pix> tile_pixels{};

            for (std::size_t row = 0; row < tile::side_length_pix; ++row) {
                for (std::size_t col = 0; col < tile::side_length_pix; ++col) {
                    const std::size_t src_row = tile_row * tile::side_length_pix + row;
                    const std::size_t src_col = tile_col * tile::side_length_pix + col;
                    tile_pixels[row * tile::side_length_pix + col] = metatile_pixels[src_row][src_col];
                }
            }

            tiles.emplace_back(tile_pixels);
        }
    }

    return tiles;
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
    // TODO: add this back once we finish anim compilation
    // For now, comment it out to expose potential bugs and help with dev
    // set_flower_key_frame_at(middle, 2);

    component->bottom(bottom);
    component->middle(middle);
    component->top(top);

    // Set metatile attributes for the sample tiles
    // TODO: add a tall grass metatile and move this to that one
    const auto behavior_result = behavior_map_->lookup("MB_TALL_GRASS");
    if (behavior_result.has_value()) {
        // Only set the behavior if the user project had an MB_TALL_GRASS behavior
        component->insert_attribute(1, MetatileAttribute{LayerType::normal, behavior_result.value()});
    }

    // Set up flower animation frames using the conversion helper
    AnimationFrame<Rgba32> flower_key{"key", metatile_array_to_tiles(flower_frame_key)};
    AnimationFrame<Rgba32> flower_0{"0", metatile_array_to_tiles(flower_frame_0)};
    AnimationFrame<Rgba32> flower_1{"1", metatile_array_to_tiles(flower_frame_1)};
    AnimationFrame<Rgba32> flower_2{"2", metatile_array_to_tiles(flower_frame_2)};

    AnimationParams flower_params{};
    flower_params.frame_names(std::vector<std::string>{"0", "1", "2"});
    flower_params.frame_order(std::vector<std::string>{"0", "1", "0", "2"});
    flower_params.width_tiles(2);
    flower_params.height_tiles(2);

    Animation<Rgba32> flower{"flower", flower_params};
    flower.key_frame(flower_key);
    flower.put_frame(flower_0.frame_name(), flower_0);
    flower.put_frame(flower_1.frame_name(), flower_1);
    flower.put_frame(flower_2.frame_name(), flower_2);
    component->add_anim(flower);

    return component;
}

} // namespace porytiles2
