#include "porytiles2/domain/services/tileset_creator.hpp"

#include <cstddef>
#include <memory>
#include <vector>

#include "porytiles2/domain/models/anim_frame.hpp"
#include "porytiles2/domain/models/animation.hpp"
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
constexpr Rgba32 mud_bulk{111, 81, 22};
constexpr Rgba32 mud_light{114, 97, 55};
constexpr Rgba32 mud_dark{111, 64, 49};
constexpr Rgba32 flower_petal{255, 197, 148};
constexpr Rgba32 flower_core{205, 65, 82};
constexpr Rgba32 flower_shadow{24, 164, 106};
constexpr Rgba32 flower_outline_light{57, 139, 49};
constexpr Rgba32 flower_outline_dark{57, 82, 0};
constexpr Rgba32 flower_leaf{131, 197, 98};
constexpr Rgba32 roof_shared_grey_light{148, 164, 180};
constexpr Rgba32 roof_shared_grey_medium{123, 123, 131};
constexpr Rgba32 roof_shared_grey_dark{90, 90, 115};
constexpr Rgba32 roof_center_bulk{255, 205, 139};
constexpr Rgba32 roof_mart_bulk{156, 213, 255};
constexpr Rgba32 roof_center_light{238, 148, 115};
constexpr Rgba32 roof_mart_light{115, 189, 246};
constexpr Rgba32 roof_center_medium{222, 106, 98};
constexpr Rgba32 roof_mart_medium{98, 164, 222};
constexpr Rgba32 roof_center_dark{205, 82, 74};
constexpr Rgba32 roof_mart_dark{74, 131, 197};

constexpr std::array grass_layer{
    // Row 0
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_dark,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_dark,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 1
    std::array{
        grass_bulk,
        grass_bulk,
        grass_light,
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
        grass_dark},

    // Row 2
    std::array{
        grass_bulk,
        grass_dark,
        grass_light,
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

    // Row 3
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
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 4
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
        grass_light,
        grass_light,
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
        grass_dark,
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_light,
        grass_bulk,
        grass_bulk},

    // Row 6
    std::array{
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_dark,
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
        grass_dark},

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
        grass_dark,
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
        grass_light},

    // Row 9
    std::array{
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
        grass_light,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 10
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
        grass_dark,
        grass_light,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk},

    // Row 11
    std::array{
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
        grass_bulk},

    // Row 13
    std::array{
        grass_bulk,
        grass_dark,
        grass_bulk,
        grass_bulk,
        grass_light,
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
        grass_dark,
        grass_bulk,
        grass_light,
        grass_bulk,
        grass_bulk,
        grass_dark,
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

constexpr std::array mud_layer{
    // Row 0
    std::array{
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_dark,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_light,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_dark,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk},

    // Row 1
    std::array{
        mud_bulk,
        mud_bulk,
        mud_light,
        mud_light,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_dark},

    // Row 2
    std::array{
        mud_bulk,
        mud_dark,
        mud_light,
        mud_light,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk},

    // Row 3
    std::array{
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_light,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk},

    // Row 4
    std::array{
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_light,
        mud_light,
        mud_bulk,
        mud_bulk},

    // Row 5
    std::array{
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_dark,
        mud_bulk,
        mud_bulk,
        mud_light,
        mud_light,
        mud_bulk,
        mud_bulk},

    // Row 6
    std::array{
        mud_bulk,
        mud_light,
        mud_bulk,
        mud_bulk,
        mud_dark,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_dark},

    // Row 7
    std::array{
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk},

    // Row 8
    std::array{
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_dark,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_dark,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_light},

    // Row 9
    std::array{
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_dark,
        mud_bulk,
        mud_bulk,
        mud_light,
        mud_light,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk},

    // Row 10
    std::array{
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_dark,
        mud_light,
        mud_light,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk},

    // Row 11
    std::array{
        mud_bulk,
        mud_light,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk},

    // Row 12
    std::array{
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_light,
        mud_light,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk},

    // Row 13
    std::array{
        mud_bulk,
        mud_dark,
        mud_bulk,
        mud_bulk,
        mud_light,
        mud_light,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk},

    // Row 14
    std::array{
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_dark,
        mud_bulk,
        mud_light,
        mud_bulk,
        mud_bulk,
        mud_dark,
        mud_bulk,
        mud_bulk,
        mud_bulk},

    // Row 15
    std::array{
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk,
        mud_bulk}};

constexpr std::array tall_grass_layer{
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
        flower_shadow,
        flower_shadow,
        grass_bulk,
        grass_bulk},

    // Row 1
    std::array{
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_shadow,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_shadow,
        grass_light,
        grass_light,
        flower_shadow,
        grass_bulk},

    // Row 2
    std::array{
        grass_bulk,
        grass_bulk,
        flower_shadow,
        grass_light,
        grass_light,
        grass_bulk,
        flower_outline_dark,
        grass_bulk,
        grass_bulk,
        flower_shadow,
        grass_light,
        grass_light,
        grass_light,
        flower_shadow,
        grass_bulk,
        grass_bulk},

    // Row 3
    std::array{
        grass_bulk,
        grass_bulk,
        grass_bulk,
        flower_outline_dark,
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        grass_bulk,
        grass_bulk,
        grass_light,
        flower_outline_dark,
        flower_shadow,
        grass_bulk,
        grass_bulk},

    // Row 4
    std::array{
        grass_bulk,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        grass_bulk},

    // Row 5
    std::array{
        flower_shadow,
        grass_light,
        grass_light,
        grass_bulk,
        flower_shadow,
        flower_outline_dark,
        flower_shadow,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_shadow,
        grass_light,
        grass_light,
        grass_light,
        flower_outline_dark},

    // Row 6
    std::array{
        flower_shadow,
        flower_shadow,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_outline_dark,
        grass_bulk,
        grass_bulk,
        grass_light,
        grass_light,
        flower_outline_dark,
        grass_bulk},

    // Row 7
    std::array{
        grass_bulk,
        flower_outline_dark,
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_outline_dark,
        flower_shadow,
        grass_bulk,
        grass_light,
        flower_shadow,
        grass_bulk,
        grass_bulk},

    // Row 8
    std::array{
        grass_bulk,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        grass_bulk},

    // Row 9
    std::array{
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_shadow},

    // Row 10
    std::array{
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        grass_bulk,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        grass_bulk,
        grass_bulk,
        flower_outline_dark,
        grass_bulk},

    // Row 11
    std::array{
        grass_bulk,
        flower_outline_dark,
        grass_light,
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_shadow,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        flower_outline_dark},

    // Row 12
    std::array{
        grass_bulk,
        flower_shadow,
        grass_bulk,
        grass_light,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow},

    // Row 13
    std::array{
        grass_bulk,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        grass_bulk,
        flower_shadow,
        grass_bulk,
        grass_bulk,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_shadow},

    // Row 14
    std::array{
        grass_bulk,
        grass_bulk,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        grass_bulk,
        flower_outline_dark,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
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
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk,
        grass_bulk}};

constexpr std::array roof_center_layer{
    // Row 0
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 1
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 2
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        roof_shared_grey_light,
        roof_shared_grey_medium,
        roof_shared_grey_medium,
        roof_shared_grey_light,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 3
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        roof_shared_grey_medium,
        roof_center_medium,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_medium,
        roof_shared_grey_medium,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 4
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        roof_shared_grey_dark,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_shared_grey_dark,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 5
    std::array{
        rgba_magenta,
        rgba_magenta,
        roof_shared_grey_dark,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_light,
        roof_center_light,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_shared_grey_dark,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 6
    std::array{
        rgba_magenta,
        roof_shared_grey_dark,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_light,
        roof_center_light,
        roof_center_light,
        roof_center_light,
        roof_center_light,
        roof_center_light,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_shared_grey_dark,
        rgba_magenta,
    },
    // Row 7
    std::array{
        roof_shared_grey_dark,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_medium,
        roof_center_medium,
        roof_center_dark,
        roof_center_dark,
        roof_center_light,
        roof_center_light,
        roof_center_dark,
        roof_center_dark,
        roof_center_medium,
        roof_center_medium,
        roof_center_bulk,
        roof_center_bulk,
        roof_shared_grey_dark,
    },
    // Row 8
    std::array{
        roof_shared_grey_dark,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_medium,
        roof_center_medium,
        roof_center_dark,
        roof_center_dark,
        roof_center_light,
        roof_center_light,
        roof_center_dark,
        roof_center_dark,
        roof_center_medium,
        roof_center_medium,
        roof_center_bulk,
        roof_center_bulk,
        roof_shared_grey_dark,
    },
    // Row 9
    std::array{
        rgba_magenta,
        roof_shared_grey_dark,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_light,
        roof_center_light,
        roof_center_light,
        roof_center_light,
        roof_center_light,
        roof_center_light,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_shared_grey_dark,
        rgba_magenta,
    },
    // Row 10
    std::array{
        rgba_magenta,
        rgba_magenta,
        roof_shared_grey_dark,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_light,
        roof_center_light,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_shared_grey_dark,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 11
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        roof_shared_grey_dark,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_shared_grey_dark,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 12
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        roof_shared_grey_medium,
        roof_center_medium,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_bulk,
        roof_center_medium,
        roof_shared_grey_medium,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 13
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        roof_shared_grey_light,
        roof_shared_grey_medium,
        roof_shared_grey_medium,
        roof_shared_grey_light,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 14
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 15
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
};

constexpr std::array roof_mart_layer{
    // Row 0
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 1
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 2
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        roof_shared_grey_light,
        roof_shared_grey_medium,
        roof_shared_grey_medium,
        roof_shared_grey_light,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 3
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        roof_shared_grey_medium,
        roof_mart_medium,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_medium,
        roof_shared_grey_medium,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 4
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        roof_shared_grey_dark,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_shared_grey_dark,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 5
    std::array{
        rgba_magenta,
        rgba_magenta,
        roof_shared_grey_dark,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_light,
        roof_mart_light,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_shared_grey_dark,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 6
    std::array{
        rgba_magenta,
        roof_shared_grey_dark,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_light,
        roof_mart_light,
        roof_mart_light,
        roof_mart_light,
        roof_mart_light,
        roof_mart_light,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_shared_grey_dark,
        rgba_magenta,
    },
    // Row 7
    std::array{
        roof_shared_grey_dark,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_medium,
        roof_mart_medium,
        roof_mart_dark,
        roof_mart_dark,
        roof_mart_light,
        roof_mart_light,
        roof_mart_dark,
        roof_mart_dark,
        roof_mart_medium,
        roof_mart_medium,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_shared_grey_dark,
    },
    // Row 8
    std::array{
        roof_shared_grey_dark,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_medium,
        roof_mart_medium,
        roof_mart_dark,
        roof_mart_dark,
        roof_mart_light,
        roof_mart_light,
        roof_mart_dark,
        roof_mart_dark,
        roof_mart_medium,
        roof_mart_medium,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_shared_grey_dark,
    },
    // Row 9
    std::array{
        rgba_magenta,
        roof_shared_grey_dark,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_light,
        roof_mart_light,
        roof_mart_light,
        roof_mart_light,
        roof_mart_light,
        roof_mart_light,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_shared_grey_dark,
        rgba_magenta,
    },
    // Row 10
    std::array{
        rgba_magenta,
        rgba_magenta,
        roof_shared_grey_dark,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_light,
        roof_mart_light,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_shared_grey_dark,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 11
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        roof_shared_grey_dark,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_shared_grey_dark,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 12
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        roof_shared_grey_medium,
        roof_mart_medium,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_bulk,
        roof_mart_medium,
        roof_shared_grey_medium,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 13
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        roof_shared_grey_light,
        roof_shared_grey_medium,
        roof_shared_grey_medium,
        roof_shared_grey_light,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 14
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
    // Row 15
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
    },
};

constexpr std::array flower_frame_key{
    // Row 0
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta},

    // Row 1
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta},

    // Row 2
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        flower_outline_light,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_outline_light,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta},

    // Row 3
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_light,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta},

    // Row 4
    std::array{
        rgba_magenta,
        rgba_magenta,
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
        rgba_magenta},

    // Row 5
    std::array{
        rgba_magenta,
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
        rgba_magenta,
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
        rgba_magenta,
        rgba_magenta,
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
        rgba_magenta},

    // Row 10
    std::array{
        rgba_magenta,
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
        rgba_magenta},

    // Row 11
    std::array{
        rgba_magenta,
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
        rgba_magenta},

    // Row 12
    std::array{
        rgba_magenta,
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
        rgba_magenta},

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
        rgba_magenta},

    // Row 14
    std::array{
        rgba_magenta,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        rgba_magenta},

    // Row 15
    std::array{
        rgba_magenta,
        rgba_magenta,
        flower_shadow,
        flower_shadow,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        flower_shadow,
        flower_shadow,
        rgba_magenta,
        rgba_magenta}};

// Frame 0 matches key frame in this case
constexpr std::array<std::array<Rgba32, metatile::side_length_pix>, metatile::side_length_pix> flower_frame_center =
    flower_frame_key;

constexpr std::array flower_frame_right{
    // Row 0
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta},

    // Row 1
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta},

    // Row 2
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        flower_outline_light,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_outline_light,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta},

    // Row 3
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_light,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta},

    // Row 4
    std::array{
        rgba_magenta,
        rgba_magenta,
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
        rgba_magenta},

    // Row 5
    std::array{
        rgba_magenta,
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
        rgba_magenta,
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
        rgba_magenta,
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
        rgba_magenta,
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
        rgba_magenta,
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
        rgba_magenta,
        rgba_magenta,
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
        rgba_magenta},

    // Row 11
    std::array{
        rgba_magenta,
        rgba_magenta,
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
        rgba_magenta},

    // Row 12
    std::array{
        rgba_magenta,
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
        rgba_magenta},

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
        rgba_magenta},

    // Row 14
    std::array{
        rgba_magenta,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        rgba_magenta},

    // Row 15
    std::array{
        rgba_magenta,
        rgba_magenta,
        flower_shadow,
        flower_shadow,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        flower_shadow,
        flower_shadow,
        rgba_magenta,
        rgba_magenta}};

constexpr std::array flower_frame_left{
    // Row 0
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta},

    // Row 1
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta},

    // Row 2
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        flower_outline_light,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_outline_light,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta},

    // Row 3
    std::array{
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        flower_outline_light,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_light,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta},

    // Row 4
    std::array{
        rgba_magenta,
        rgba_magenta,
        flower_outline_light,
        flower_outline_dark,
        flower_petal,
        flower_petal,
        flower_core,
        flower_core,
        flower_petal,
        flower_petal,
        flower_outline_dark,
        rgba_magenta,
        flower_outline_light,
        flower_outline_light,
        flower_outline_light,
        rgba_magenta},

    // Row 5
    std::array{
        rgba_magenta,
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
        rgba_magenta,
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
        rgba_magenta},

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
        rgba_magenta},

    // Row 11
    std::array{
        rgba_magenta,
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
        rgba_magenta},

    // Row 12
    std::array{
        rgba_magenta,
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
        rgba_magenta},

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
        rgba_magenta},

    // Row 14
    std::array{
        rgba_magenta,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        flower_shadow,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        flower_shadow,
        flower_shadow,
        flower_shadow,
        flower_outline_dark,
        flower_outline_dark,
        flower_shadow,
        rgba_magenta},

    // Row 15
    std::array{
        rgba_magenta,
        rgba_magenta,
        flower_shadow,
        flower_shadow,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        rgba_magenta,
        flower_shadow,
        flower_shadow,
        rgba_magenta,
        rgba_magenta}};

void set_grass_at(Image<Rgba32> &img, std::size_t metatile_index)
{
    const std::size_t col_offset = metatile_index * metatile::side_length_pix;
    for (std::size_t row = 0; row < metatile::side_length_pix; ++row) {
        for (std::size_t col = 0; col < metatile::side_length_pix; ++col) {
            img.set(row, col_offset + col, grass_layer[row][col]);
        }
    }
}

void set_mud_at(Image<Rgba32> &img, std::size_t metatile_index)
{
    const std::size_t col_offset = metatile_index * metatile::side_length_pix;
    for (std::size_t row = 0; row < metatile::side_length_pix; ++row) {
        for (std::size_t col = 0; col < metatile::side_length_pix; ++col) {
            img.set(row, col_offset + col, mud_layer[row][col]);
        }
    }
}

void set_tall_grass_at(Image<Rgba32> &img, std::size_t metatile_index)
{
    const std::size_t col_offset = metatile_index * metatile::side_length_pix;
    for (std::size_t row = 0; row < metatile::side_length_pix; ++row) {
        for (std::size_t col = 0; col < metatile::side_length_pix; ++col) {
            img.set(row, col_offset + col, tall_grass_layer[row][col]);
        }
    }
}

void set_roof_center_at(Image<Rgba32> &img, std::size_t metatile_index)
{
    const std::size_t col_offset = metatile_index * metatile::side_length_pix;
    for (std::size_t row = 0; row < metatile::side_length_pix; ++row) {
        for (std::size_t col = 0; col < metatile::side_length_pix; ++col) {
            img.set(row, col_offset + col, roof_center_layer[row][col]);
        }
    }
}

void set_roof_mart_at(Image<Rgba32> &img, std::size_t metatile_index)
{
    const std::size_t col_offset = metatile_index * metatile::side_length_pix;
    for (std::size_t row = 0; row < metatile::side_length_pix; ++row) {
        for (std::size_t col = 0; col < metatile::side_length_pix; ++col) {
            img.set(row, col_offset + col, roof_mart_layer[row][col]);
        }
    }
}

void set_flower_key_frame_at(Image<Rgba32> &img, std::size_t metatile_index)
{
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
TilesetCreator::create_sample_primary_porytiles_component(const std::string &tileset_name) const
{
    PT_UNWRAP_TILESET_CONFIG_PTR(
        config_, extrinsic_transparency, tileset_name, std::unique_ptr<PorytilesTilesetComponent>);

    auto component = std::make_unique<PorytilesTilesetComponent>();

    Image<Rgba32> bottom{128, 16, extrinsic_transparency};
    Image<Rgba32> middle{128, 16, extrinsic_transparency};
    Image<Rgba32> top{128, 16, extrinsic_transparency};

    set_grass_at(middle, 1);
    set_flower_key_frame_at(middle, 2);
    set_grass_at(bottom, 2);
    set_tall_grass_at(middle, 3);
    set_grass_at(bottom, 4);
    set_roof_center_at(middle, 4);
    set_grass_at(bottom, 5);
    set_roof_mart_at(middle, 5);

    component->bottom(bottom);
    component->middle(middle);
    component->top(top);

    /*
     * Set metatile attribute for the tall grass metatile. But only set the behavior if the user project had an
     * MB_TALL_GRASS behavior.
     */
    const auto behavior_result = behavior_map_->lookup("MB_TALL_GRASS");
    if (behavior_result.has_value()) {
        component->insert_attribute(3, MetatileAttribute{LayerType::normal, behavior_result.value()});
    }

    // Set up flower animation frames using the conversion helper
    AnimFrame flower_key{"key", metatile_array_to_tiles(flower_frame_key)};
    AnimFrame flower_center{"center", metatile_array_to_tiles(flower_frame_center)};
    AnimFrame flower_right{"right", metatile_array_to_tiles(flower_frame_right)};
    AnimFrame flower_left{"left", metatile_array_to_tiles(flower_frame_left)};

    AnimParams flower_params{};
    flower_params.frame_names(
        std::vector{DynamicCasedName{"center"}, DynamicCasedName{"right"}, DynamicCasedName{"left"}});
    flower_params.frame_order(
        std::vector{
            DynamicCasedName{"center"},
            DynamicCasedName{"right"},
            DynamicCasedName{"center"},
            DynamicCasedName{"left"}});
    flower_params.width_tiles(2);
    flower_params.height_tiles(2);

    Animation<Rgba32> flower{"flower", flower_params};
    flower.key_frame(flower_key);
    flower.put_frame(flower_center.frame_name(), flower_center);
    flower.put_frame(flower_right.frame_name(), flower_right);
    flower.put_frame(flower_left.frame_name(), flower_left);
    component->add_anim(flower);

    return component;
}

ChainableResult<std::unique_ptr<PorytilesTilesetComponent>>
TilesetCreator::create_sample_secondary_porytiles_component(const std::string &tileset_name) const
{
    PT_UNWRAP_TILESET_CONFIG_PTR(
        config_, extrinsic_transparency, tileset_name, std::unique_ptr<PorytilesTilesetComponent>);

    auto component = std::make_unique<PorytilesTilesetComponent>();

    Image<Rgba32> bottom{128, 16, extrinsic_transparency};
    Image<Rgba32> middle{128, 16, extrinsic_transparency};
    Image<Rgba32> top{128, 16, extrinsic_transparency};

    set_mud_at(middle, 1);
    set_mud_at(bottom, 2);
    set_flower_key_frame_at(middle, 2);
    set_grass_at(middle, 3);

    component->bottom(bottom);
    component->middle(middle);
    component->top(top);

    return component;
}

} // namespace porytiles2
