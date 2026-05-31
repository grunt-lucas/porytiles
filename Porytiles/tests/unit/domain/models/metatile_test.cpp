#include <gtest/gtest.h>

#include <tuple>

#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/rgba32.hpp"

using namespace porytiles;
using namespace porytiles::metatile;

[[nodiscard]] PixelTile<IndexPixel> create_nontransparent_index_tile()
{
    std::array<IndexPixel, tile::size_pix> pixels{};
    pixels[0] = IndexPixel{1}; // Set first pixel to non-transparent
    return PixelTile<IndexPixel>{pixels};
}

[[nodiscard]] PixelTile<Rgba32> create_nontransparent_rgba_tile()
{
    std::array<Rgba32, tile::size_pix> pixels{};
    pixels[0] = Rgba32{255, 0, 0, 255}; // Set first pixel to red (non-transparent)
    return PixelTile<Rgba32>{pixels};
}

TEST(MetatileNamespaceTests, FromTileBottomLayer)
{
    {
        auto [mt, layer, subtile] = from_tile_index(0);
        EXPECT_EQ(mt, 0);
        EXPECT_EQ(layer, Layer::bottom);
        EXPECT_EQ(subtile, Subtile::northwest);
    }
    {
        auto [mt, layer, subtile] = from_tile_index(1);
        EXPECT_EQ(mt, 0);
        EXPECT_EQ(layer, Layer::bottom);
        EXPECT_EQ(subtile, Subtile::northeast);
    }
    {
        auto [mt, layer, subtile] = from_tile_index(2);
        EXPECT_EQ(mt, 0);
        EXPECT_EQ(layer, Layer::bottom);
        EXPECT_EQ(subtile, Subtile::southwest);
    }
    {
        auto [mt, layer, subtile] = from_tile_index(3);
        EXPECT_EQ(mt, 0);
        EXPECT_EQ(layer, Layer::bottom);
        EXPECT_EQ(subtile, Subtile::southeast);
    }
}

TEST(MetatileNamespaceTests, FromTileMiddleLayer)
{
    {
        auto [mt, layer, subtile] = from_tile_index(4);
        EXPECT_EQ(mt, 0);
        EXPECT_EQ(layer, Layer::middle);
        EXPECT_EQ(subtile, Subtile::northwest);
    }
    {
        auto [mt, layer, subtile] = from_tile_index(7);
        EXPECT_EQ(mt, 0);
        EXPECT_EQ(layer, Layer::middle);
        EXPECT_EQ(subtile, Subtile::southeast);
    }
}

TEST(MetatileNamespaceTests, FromTileTopLayer)
{
    {
        auto [mt, layer, subtile] = from_tile_index(8);
        EXPECT_EQ(mt, 0);
        EXPECT_EQ(layer, Layer::top);
        EXPECT_EQ(subtile, Subtile::northwest);
    }
    {
        auto [mt, layer, subtile] = from_tile_index(11);
        EXPECT_EQ(mt, 0);
        EXPECT_EQ(layer, Layer::top);
        EXPECT_EQ(subtile, Subtile::southeast);
    }
}

TEST(MetatileNamespaceTests, FromTileSecondMetatile)
{
    {
        auto [mt, layer, subtile] = from_tile_index(12);
        EXPECT_EQ(mt, 1);
        EXPECT_EQ(layer, Layer::bottom);
        EXPECT_EQ(subtile, Subtile::northwest);
    }
    {
        auto [mt, layer, subtile] = from_tile_index(16);
        EXPECT_EQ(mt, 1);
        EXPECT_EQ(layer, Layer::middle);
        EXPECT_EQ(subtile, Subtile::northwest);
    }
    {
        auto [mt, layer, subtile] = from_tile_index(23);
        EXPECT_EQ(mt, 1);
        EXPECT_EQ(layer, Layer::top);
        EXPECT_EQ(subtile, Subtile::southeast);
    }
}

TEST(MetatileNamespaceTests, FromTileHigherIndices)
{
    {
        auto [mt, layer, subtile] = from_tile_index(36);
        EXPECT_EQ(mt, 3);
        EXPECT_EQ(layer, Layer::bottom);
        EXPECT_EQ(subtile, Subtile::northwest);
    }
    {
        // 96 + 4 + 2
        auto [mt, layer, subtile] = from_tile_index(102);
        EXPECT_EQ(mt, 8);
        EXPECT_EQ(layer, Layer::middle);
        EXPECT_EQ(subtile, Subtile::southwest);
    }
}

TEST(MetatileNamespaceTests, FromInternalTileBottomLayer)
{
    {
        auto [layer, subtile] = from_internal_tile_index(0);
        EXPECT_EQ(layer, Layer::bottom);
        EXPECT_EQ(subtile, Subtile::northwest);
    }
    {
        auto [layer, subtile] = from_internal_tile_index(1);
        EXPECT_EQ(layer, Layer::bottom);
        EXPECT_EQ(subtile, Subtile::northeast);
    }
    {
        auto [layer, subtile] = from_internal_tile_index(2);
        EXPECT_EQ(layer, Layer::bottom);
        EXPECT_EQ(subtile, Subtile::southwest);
    }
    {
        auto [layer, subtile] = from_internal_tile_index(3);
        EXPECT_EQ(layer, Layer::bottom);
        EXPECT_EQ(subtile, Subtile::southeast);
    }
}

TEST(MetatileNamespaceTests, FromInternalTileMiddleLayer)
{
    {
        auto [layer, subtile] = from_internal_tile_index(4);
        EXPECT_EQ(layer, Layer::middle);
        EXPECT_EQ(subtile, Subtile::northwest);
    }
    {
        auto [layer, subtile] = from_internal_tile_index(5);
        EXPECT_EQ(layer, Layer::middle);
        EXPECT_EQ(subtile, Subtile::northeast);
    }
    {
        auto [layer, subtile] = from_internal_tile_index(6);
        EXPECT_EQ(layer, Layer::middle);
        EXPECT_EQ(subtile, Subtile::southwest);
    }
    {
        auto [layer, subtile] = from_internal_tile_index(7);
        EXPECT_EQ(layer, Layer::middle);
        EXPECT_EQ(subtile, Subtile::southeast);
    }
}

TEST(MetatileNamespaceTests, FromInternalTileTopLayer)
{
    {
        auto [layer, subtile] = from_internal_tile_index(8);
        EXPECT_EQ(layer, Layer::top);
        EXPECT_EQ(subtile, Subtile::northwest);
    }
    {
        auto [layer, subtile] = from_internal_tile_index(9);
        EXPECT_EQ(layer, Layer::top);
        EXPECT_EQ(subtile, Subtile::northeast);
    }
    {
        auto [layer, subtile] = from_internal_tile_index(10);
        EXPECT_EQ(layer, Layer::top);
        EXPECT_EQ(subtile, Subtile::southwest);
    }
    {
        auto [layer, subtile] = from_internal_tile_index(11);
        EXPECT_EQ(layer, Layer::top);
        EXPECT_EQ(subtile, Subtile::southeast);
    }
}

TEST(MetatileNamespaceTests, FromInternalTilePanicsOnOutOfBounds)
{
    EXPECT_DEATH({ std::ignore = from_internal_tile_index(tiles_per_metatile); }, "tile_index.*>=.*tiles_per_metatile");
    EXPECT_DEATH({ std::ignore = from_internal_tile_index(100); }, "tile_index.*>=.*tiles_per_metatile");
}

TEST(MetatileTests, DefaultIsTransparent)
{
    // Test with IndexPixel
    const Metatile<IndexPixel> metatile_index{};
    EXPECT_TRUE(metatile_index.is_transparent());

    // Test with Rgba32
    const Metatile<Rgba32> metatile_rgba{};
    const Rgba32 transparent_ref{0, 0, 0, Rgba32::alpha_transparent};
    EXPECT_TRUE(metatile_rgba.is_transparent(transparent_ref));
}

TEST(MetatileTests, InferLayerModeAllLayersWithContent_IndexPixel)
{
    Metatile<IndexPixel> metatile{};
    metatile.set_bottom(0, create_nontransparent_index_tile());
    metatile.set_middle(0, create_nontransparent_index_tile());
    metatile.set_top(0, create_nontransparent_index_tile());

    EXPECT_EQ(metatile.infer_layer_mode(), LayerMode::triple);
}

TEST(MetatileTests, InferLayerModeAllLayersWithContent_Rgba32)
{
    Metatile<Rgba32> metatile{};
    metatile.set_bottom(0, create_nontransparent_rgba_tile());
    metatile.set_middle(0, create_nontransparent_rgba_tile());
    metatile.set_top(0, create_nontransparent_rgba_tile());

    const Rgba32 extrinsic{255, 0, 255, 255}; // Magenta as transparency key
    EXPECT_EQ(metatile.infer_layer_mode(extrinsic), LayerMode::triple);
}

TEST(MetatileTests, InferLayerModeTwoLayersWithContent_IndexPixel)
{
    Metatile<IndexPixel> metatile{};
    metatile.set_bottom(0, create_nontransparent_index_tile());
    metatile.set_middle(0, create_nontransparent_index_tile());
    // Top layer remains transparent

    EXPECT_EQ(metatile.infer_layer_mode(), LayerMode::dual);
}

TEST(MetatileTests, InferLayerModeOneLayerWithContent_IndexPixel)
{
    Metatile<IndexPixel> metatile{};
    metatile.set_bottom(0, create_nontransparent_index_tile());
    // Middle and top layers remain transparent

    EXPECT_EQ(metatile.infer_layer_mode(), LayerMode::dual);
}

TEST(MetatileTests, InferLayerModeNoContent_IndexPixel)
{
    const Metatile<IndexPixel> metatile{}; // All layers transparent

    EXPECT_EQ(metatile.infer_layer_mode(), LayerMode::dual);
}

TEST(MetatileTests, InferLayerTypeTripleMode_IndexPixel)
{
    Metatile<IndexPixel> metatile{};
    metatile.set_bottom(0, create_nontransparent_index_tile());
    metatile.set_middle(0, create_nontransparent_index_tile());
    metatile.set_top(0, create_nontransparent_index_tile());

    // Triple mode always returns normal
    EXPECT_EQ(metatile.infer_layer_type(), LayerType::normal);
}

TEST(MetatileTests, InferLayerTypeNoContent_IndexPixel)
{
    // Case 0: completely transparent -> normal
    const Metatile<IndexPixel> metatile{};

    EXPECT_EQ(metatile.infer_layer_type(), LayerType::normal);
}

TEST(MetatileTests, InferLayerTypeBottomOnly_IndexPixel)
{
    // Case 1: bottom only -> covered
    Metatile<IndexPixel> metatile{};
    metatile.set_bottom(0, create_nontransparent_index_tile());

    EXPECT_EQ(metatile.infer_layer_type(), LayerType::covered);
}

TEST(MetatileTests, InferLayerTypeMiddleOnly_IndexPixel)
{
    // Case 2: middle only -> normal
    Metatile<IndexPixel> metatile{};
    metatile.set_middle(0, create_nontransparent_index_tile());

    EXPECT_EQ(metatile.infer_layer_type(), LayerType::normal);
}

TEST(MetatileTests, InferLayerTypeTopOnly_IndexPixel)
{
    // Case 3: top only -> normal
    Metatile<IndexPixel> metatile{};
    metatile.set_top(0, create_nontransparent_index_tile());

    EXPECT_EQ(metatile.infer_layer_type(), LayerType::normal);
}

TEST(MetatileTests, InferLayerTypeMiddleAndTop_IndexPixel)
{
    // Case 4: middle/top content -> normal
    Metatile<IndexPixel> metatile{};
    metatile.set_middle(0, create_nontransparent_index_tile());
    metatile.set_top(0, create_nontransparent_index_tile());

    EXPECT_EQ(metatile.infer_layer_type(), LayerType::normal);
}

TEST(MetatileTests, InferLayerTypeBottomAndMiddle_IndexPixel)
{
    // Case 5: bottom/middle content -> covered
    Metatile<IndexPixel> metatile{};
    metatile.set_bottom(0, create_nontransparent_index_tile());
    metatile.set_middle(0, create_nontransparent_index_tile());

    EXPECT_EQ(metatile.infer_layer_type(), LayerType::covered);
}

TEST(MetatileTests, InferLayerTypeBottomAndTop_IndexPixel)
{
    // Case 6: bottom/top content -> split
    Metatile<IndexPixel> metatile{};
    metatile.set_bottom(0, create_nontransparent_index_tile());
    metatile.set_top(0, create_nontransparent_index_tile());

    EXPECT_EQ(metatile.infer_layer_type(), LayerType::split);
}

TEST(MetatileTests, InferLayerTypeWithExtrinsicTransparency_Rgba32)
{
    // Test with Rgba32 to verify extrinsic transparency handling
    Metatile<Rgba32> metatile{};
    metatile.set_bottom(0, create_nontransparent_rgba_tile());
    metatile.set_top(0, create_nontransparent_rgba_tile());

    const Rgba32 extrinsic{255, 0, 255, 255}; // Magenta as transparency key
    EXPECT_EQ(metatile.infer_layer_type(extrinsic), LayerType::split);
}
