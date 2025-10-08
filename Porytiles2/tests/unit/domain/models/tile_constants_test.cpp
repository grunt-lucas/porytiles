#include <gtest/gtest.h>

#include "porytiles2/domain/models/tile_constants.hpp"

using namespace porytiles2::metatile;

TEST(TileConstantsTests, ComputeMetatileBottomLayer)
{
    // Tile 0: metatile 0, bottom layer, northwest
    auto [mt0, layer0, subtile0] = compute_metatile(0);
    EXPECT_EQ(mt0, 0);
    EXPECT_EQ(layer0, Layer::bottom);
    EXPECT_EQ(subtile0, Subtile::northwest);

    // Tile 1: metatile 0, bottom layer, northeast
    auto [mt1, layer1, subtile1] = compute_metatile(1);
    EXPECT_EQ(mt1, 0);
    EXPECT_EQ(layer1, Layer::bottom);
    EXPECT_EQ(subtile1, Subtile::northeast);

    // Tile 2: metatile 0, bottom layer, southwest
    auto [mt2, layer2, subtile2] = compute_metatile(2);
    EXPECT_EQ(mt2, 0);
    EXPECT_EQ(layer2, Layer::bottom);
    EXPECT_EQ(subtile2, Subtile::southwest);

    // Tile 3: metatile 0, bottom layer, southeast
    auto [mt3, layer3, subtile3] = compute_metatile(3);
    EXPECT_EQ(mt3, 0);
    EXPECT_EQ(layer3, Layer::bottom);
    EXPECT_EQ(subtile3, Subtile::southeast);
}

TEST(TileConstantsTests, ComputeMetatileMiddleLayer)
{
    // Tile 4: metatile 0, middle layer, northwest
    auto [mt0, layer0, subtile0] = compute_metatile(4);
    EXPECT_EQ(mt0, 0);
    EXPECT_EQ(layer0, Layer::middle);
    EXPECT_EQ(subtile0, Subtile::northwest);

    // Tile 7: metatile 0, middle layer, southeast
    auto [mt1, layer1, subtile1] = compute_metatile(7);
    EXPECT_EQ(mt1, 0);
    EXPECT_EQ(layer1, Layer::middle);
    EXPECT_EQ(subtile1, Subtile::southeast);
}

TEST(TileConstantsTests, ComputeMetatileTopLayer)
{
    // Tile 8: metatile 0, top layer, northwest
    auto [mt0, layer0, subtile0] = compute_metatile(8);
    EXPECT_EQ(mt0, 0);
    EXPECT_EQ(layer0, Layer::top);
    EXPECT_EQ(subtile0, Subtile::northwest);

    // Tile 11: metatile 0, top layer, southeast
    auto [mt1, layer1, subtile1] = compute_metatile(11);
    EXPECT_EQ(mt1, 0);
    EXPECT_EQ(layer1, Layer::top);
    EXPECT_EQ(subtile1, Subtile::southeast);
}

TEST(TileConstantsTests, ComputeMetatileSecondMetatile)
{
    // Tile 12: metatile 1, bottom layer, northwest
    auto [mt0, layer0, subtile0] = compute_metatile(12);
    EXPECT_EQ(mt0, 1);
    EXPECT_EQ(layer0, Layer::bottom);
    EXPECT_EQ(subtile0, Subtile::northwest);

    // Tile 16: metatile 1, middle layer, northwest
    auto [mt1, layer1, subtile1] = compute_metatile(16);
    EXPECT_EQ(mt1, 1);
    EXPECT_EQ(layer1, Layer::middle);
    EXPECT_EQ(subtile1, Subtile::northwest);

    // Tile 23: metatile 1, top layer, southeast
    auto [mt2, layer2, subtile2] = compute_metatile(23);
    EXPECT_EQ(mt2, 1);
    EXPECT_EQ(layer2, Layer::top);
    EXPECT_EQ(subtile2, Subtile::southeast);
}

TEST(TileConstantsTests, ComputeMetatileHigherIndices)
{
    // Tile 36: metatile 3, bottom layer, northwest
    auto [mt0, layer0, subtile0] = compute_metatile(36);
    EXPECT_EQ(mt0, 3);
    EXPECT_EQ(layer0, Layer::bottom);
    EXPECT_EQ(subtile0, Subtile::northwest);

    // Tile 102: metatile 8, middle layer, southwest (96 + 4 + 2)
    auto [mt1, layer1, subtile1] = compute_metatile(102);
    EXPECT_EQ(mt1, 8);
    EXPECT_EQ(layer1, Layer::middle);
    EXPECT_EQ(subtile1, Subtile::southwest);
}
