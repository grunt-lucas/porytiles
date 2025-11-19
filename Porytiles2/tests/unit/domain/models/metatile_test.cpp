#include <gtest/gtest.h>

#include <tuple>

#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"

using namespace porytiles2;
using namespace porytiles2::metatile;

// Helper function to create a non-transparent IndexPixel tile
[[nodiscard]] PixelTile<IndexPixel> create_nontransparent_index_tile()
{
    std::array<IndexPixel, tile::size_pix> pixels{};
    pixels[0] = IndexPixel{1}; // Set first pixel to non-transparent
    return PixelTile<IndexPixel>{pixels};
}

// Helper function to create a non-transparent Rgba32 tile
[[nodiscard]] PixelTile<Rgba32> create_nontransparent_rgba_tile()
{
    std::array<Rgba32, tile::size_pix> pixels{};
    pixels[0] = Rgba32{255, 0, 0, 255}; // Set first pixel to red (non-transparent)
    return PixelTile<Rgba32>{pixels};
}

TEST(MetatileNamespaceTests, FromTileBottomLayer)
{
    // Tile 0: metatile 0, bottom layer, northwest
    auto [mt0, layer0, subtile0] = from_tile_index(0);
    EXPECT_EQ(mt0, 0);
    EXPECT_EQ(layer0, Layer::bottom);
    EXPECT_EQ(subtile0, Subtile::northwest);

    // Tile 1: metatile 0, bottom layer, northeast
    auto [mt1, layer1, subtile1] = from_tile_index(1);
    EXPECT_EQ(mt1, 0);
    EXPECT_EQ(layer1, Layer::bottom);
    EXPECT_EQ(subtile1, Subtile::northeast);

    // Tile 2: metatile 0, bottom layer, southwest
    auto [mt2, layer2, subtile2] = from_tile_index(2);
    EXPECT_EQ(mt2, 0);
    EXPECT_EQ(layer2, Layer::bottom);
    EXPECT_EQ(subtile2, Subtile::southwest);

    // Tile 3: metatile 0, bottom layer, southeast
    auto [mt3, layer3, subtile3] = from_tile_index(3);
    EXPECT_EQ(mt3, 0);
    EXPECT_EQ(layer3, Layer::bottom);
    EXPECT_EQ(subtile3, Subtile::southeast);
}

TEST(MetatileNamespaceTests, FromTileMiddleLayer)
{
    // Tile 4: metatile 0, middle layer, northwest
    auto [mt0, layer0, subtile0] = from_tile_index(4);
    EXPECT_EQ(mt0, 0);
    EXPECT_EQ(layer0, Layer::middle);
    EXPECT_EQ(subtile0, Subtile::northwest);

    // Tile 7: metatile 0, middle layer, southeast
    auto [mt1, layer1, subtile1] = from_tile_index(7);
    EXPECT_EQ(mt1, 0);
    EXPECT_EQ(layer1, Layer::middle);
    EXPECT_EQ(subtile1, Subtile::southeast);
}

TEST(MetatileNamespaceTests, FromTileTopLayer)
{
    // Tile 8: metatile 0, top layer, northwest
    auto [mt0, layer0, subtile0] = from_tile_index(8);
    EXPECT_EQ(mt0, 0);
    EXPECT_EQ(layer0, Layer::top);
    EXPECT_EQ(subtile0, Subtile::northwest);

    // Tile 11: metatile 0, top layer, southeast
    auto [mt1, layer1, subtile1] = from_tile_index(11);
    EXPECT_EQ(mt1, 0);
    EXPECT_EQ(layer1, Layer::top);
    EXPECT_EQ(subtile1, Subtile::southeast);
}

TEST(MetatileNamespaceTests, FromTileSecondMetatile)
{
    // Tile 12: metatile 1, bottom layer, northwest
    auto [mt0, layer0, subtile0] = from_tile_index(12);
    EXPECT_EQ(mt0, 1);
    EXPECT_EQ(layer0, Layer::bottom);
    EXPECT_EQ(subtile0, Subtile::northwest);

    // Tile 16: metatile 1, middle layer, northwest
    auto [mt1, layer1, subtile1] = from_tile_index(16);
    EXPECT_EQ(mt1, 1);
    EXPECT_EQ(layer1, Layer::middle);
    EXPECT_EQ(subtile1, Subtile::northwest);

    // Tile 23: metatile 1, top layer, southeast
    auto [mt2, layer2, subtile2] = from_tile_index(23);
    EXPECT_EQ(mt2, 1);
    EXPECT_EQ(layer2, Layer::top);
    EXPECT_EQ(subtile2, Subtile::southeast);
}

TEST(MetatileNamespaceTests, FromTileHigherIndices)
{
    // Tile 36: metatile 3, bottom layer, northwest
    auto [mt0, layer0, subtile0] = from_tile_index(36);
    EXPECT_EQ(mt0, 3);
    EXPECT_EQ(layer0, Layer::bottom);
    EXPECT_EQ(subtile0, Subtile::northwest);

    // Tile 102: metatile 8, middle layer, southwest (96 + 4 + 2)
    auto [mt1, layer1, subtile1] = from_tile_index(102);
    EXPECT_EQ(mt1, 8);
    EXPECT_EQ(layer1, Layer::middle);
    EXPECT_EQ(subtile1, Subtile::southwest);
}

TEST(MetatileNamespaceTests, FromInternalTileBottomLayer)
{
    // Tile 0: bottom layer, northwest
    auto [layer0, subtile0] = from_internal_tile_index(0);
    EXPECT_EQ(layer0, Layer::bottom);
    EXPECT_EQ(subtile0, Subtile::northwest);

    // Tile 1: bottom layer, northeast
    auto [layer1, subtile1] = from_internal_tile_index(1);
    EXPECT_EQ(layer1, Layer::bottom);
    EXPECT_EQ(subtile1, Subtile::northeast);

    // Tile 2: bottom layer, southwest
    auto [layer2, subtile2] = from_internal_tile_index(2);
    EXPECT_EQ(layer2, Layer::bottom);
    EXPECT_EQ(subtile2, Subtile::southwest);

    // Tile 3: bottom layer, southeast
    auto [layer3, subtile3] = from_internal_tile_index(3);
    EXPECT_EQ(layer3, Layer::bottom);
    EXPECT_EQ(subtile3, Subtile::southeast);
}

TEST(MetatileNamespaceTests, FromInternalTileMiddleLayer)
{
    // Tile 4: middle layer, northwest
    auto [layer0, subtile0] = from_internal_tile_index(4);
    EXPECT_EQ(layer0, Layer::middle);
    EXPECT_EQ(subtile0, Subtile::northwest);

    // Tile 5: middle layer, northeast
    auto [layer1, subtile1] = from_internal_tile_index(5);
    EXPECT_EQ(layer1, Layer::middle);
    EXPECT_EQ(subtile1, Subtile::northeast);

    // Tile 6: middle layer, southwest
    auto [layer2, subtile2] = from_internal_tile_index(6);
    EXPECT_EQ(layer2, Layer::middle);
    EXPECT_EQ(subtile2, Subtile::southwest);

    // Tile 7: middle layer, southeast
    auto [layer3, subtile3] = from_internal_tile_index(7);
    EXPECT_EQ(layer3, Layer::middle);
    EXPECT_EQ(subtile3, Subtile::southeast);
}

TEST(MetatileNamespaceTests, FromInternalTileTopLayer)
{
    // Tile 8: top layer, northwest
    auto [layer0, subtile0] = from_internal_tile_index(8);
    EXPECT_EQ(layer0, Layer::top);
    EXPECT_EQ(subtile0, Subtile::northwest);

    // Tile 9: top layer, northeast
    auto [layer1, subtile1] = from_internal_tile_index(9);
    EXPECT_EQ(layer1, Layer::top);
    EXPECT_EQ(subtile1, Subtile::northeast);

    // Tile 10: top layer, southwest
    auto [layer2, subtile2] = from_internal_tile_index(10);
    EXPECT_EQ(layer2, Layer::top);
    EXPECT_EQ(subtile2, Subtile::southwest);

    // Tile 11: top layer, southeast
    auto [layer3, subtile3] = from_internal_tile_index(11);
    EXPECT_EQ(layer3, Layer::top);
    EXPECT_EQ(subtile3, Subtile::southeast);
}

TEST(MetatileNamespaceTests, FromInternalTilePanicsOnOutOfBounds)
{
    // Should panic when tile_index == tiles_per_metatile (12)
    EXPECT_DEATH({ std::ignore = from_internal_tile_index(tiles_per_metatile); }, "tile_index.*>=.*tiles_per_metatile");

    // Should also panic for higher values
    EXPECT_DEATH({ std::ignore = from_internal_tile_index(100); }, "tile_index.*>=.*tiles_per_metatile");
}

TEST(MetatileTests, DefaultConstructedValueShouldBeTransparent)
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
