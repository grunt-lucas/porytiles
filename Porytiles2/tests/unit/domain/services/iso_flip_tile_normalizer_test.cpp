#include "gtest/gtest.h"

#include "porytiles2/domain/services/iso_flip_tile_normalizer.hpp"

using namespace porytiles2;

TEST(IsoFlipTileNormalizerTests, ZeroShouldBeZero)
{
    IsoFlipTileNormalizer foo{};
    EXPECT_EQ(0, 0);
}
