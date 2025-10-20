#include "gtest/gtest.h"

#include <map>

#include "porytiles2/domain/models/color_set.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/services/color_set_builder.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"

using namespace porytiles2;

class ColorSetBuilderTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;
    ColorSetBuilder builder_{&formatter_};
};

// TEST_F(ColorSetBuilderTest, EmptyPalette)
// {
//     NormalizedPal pal{rgba_black};
//     std::map<Rgba32, unsigned int> color_index_map{{rgba_red, 0}, {rgba_green, 1}, {rgba_blue, 2}};
//
//     ColorSet result = builder_.build(pal, color_index_map);
//
//     // No colors in palette, so no bits should be set
//     for (std::size_t i = 0; i < 240; ++i) {
//         EXPECT_FALSE(result.test(i));
//     }
// }
//
// TEST_F(ColorSetBuilderTest, SingleColor)
// {
//     NormalizedPal pal{rgba_black};
//     pal.insert(rgba_red);
//
//     std::map<Rgba32, unsigned int> color_index_map{{rgba_red, 5}, {rgba_green, 10}, {rgba_blue, 15}};
//
//     ColorSet result = builder_.build(pal, color_index_map);
//
//     // Only bit at index 5 should be set
//     EXPECT_TRUE(result.test(5));
//     EXPECT_FALSE(result.test(0));
//     EXPECT_FALSE(result.test(10));
//     EXPECT_FALSE(result.test(15));
// }
//
// TEST_F(ColorSetBuilderTest, MultipleColorsAtVariousIndices)
// {
//     NormalizedPal pal{rgba_black};
//     pal.insert(rgba_red);
//     pal.insert(rgba_blue);
//     pal.insert(rgba_yellow);
//
//     std::map<Rgba32, unsigned int> color_index_map{
//         {rgba_red, 2}, {rgba_green, 5}, {rgba_blue, 11}, {rgba_yellow, 100}, {rgba_magenta, 150}};
//
//     ColorSet result = builder_.build(pal, color_index_map);
//
//     // Bits at indices 2, 11, and 100 should be set
//     EXPECT_TRUE(result.test(2));
//     EXPECT_TRUE(result.test(11));
//     EXPECT_TRUE(result.test(100));
//
//     // Other indices should not be set
//     EXPECT_FALSE(result.test(0));
//     EXPECT_FALSE(result.test(5));
//     EXPECT_FALSE(result.test(150));
// }
//
// TEST_F(ColorSetBuilderTest, VerifyBitsAtCorrectIndices)
// {
//     NormalizedPal pal{rgba_black};
//     pal.insert(rgba_cyan);
//     pal.insert(rgba_magenta);
//
//     std::map<Rgba32, unsigned int> color_index_map{{rgba_cyan, 0}, {rgba_magenta, 239}};
//
//     ColorSet result = builder_.build(pal, color_index_map);
//
//     // Test edge cases: first and last possible indices
//     EXPECT_TRUE(result.test(0));
//     EXPECT_TRUE(result.test(239));
//
//     // All other bits should be false
//     for (std::size_t i = 1; i < 239; ++i) {
//         EXPECT_FALSE(result.test(i));
//     }
// }
//
// TEST_F(ColorSetBuilderTest, AllColorsInMap)
// {
//     NormalizedPal pal{rgba_black};
//     pal.insert(rgba_red);
//     pal.insert(rgba_green);
//     pal.insert(rgba_blue);
//     pal.insert(rgba_yellow);
//     pal.insert(rgba_cyan);
//     pal.insert(rgba_magenta);
//
//     std::map<Rgba32, unsigned int> color_index_map{
//         {rgba_red, 0}, {rgba_green, 1}, {rgba_blue, 2}, {rgba_yellow, 3}, {rgba_cyan, 4}, {rgba_magenta, 5}};
//
//     ColorSet result = builder_.build(pal, color_index_map);
//
//     // All six colors should have their bits set
//     EXPECT_TRUE(result.test(0));
//     EXPECT_TRUE(result.test(1));
//     EXPECT_TRUE(result.test(2));
//     EXPECT_TRUE(result.test(3));
//     EXPECT_TRUE(result.test(4));
//     EXPECT_TRUE(result.test(5));
//
//     // Remaining bits should be false
//     for (std::size_t i = 6; i < 240; ++i) {
//         EXPECT_FALSE(result.test(i));
//     }
// }
//
// TEST_F(ColorSetBuilderTest, NonConsecutiveIndices)
// {
//     NormalizedPal pal{rgba_black};
//     pal.insert(rgba_purple);
//     pal.insert(rgba_lime);
//
//     std::map<Rgba32, unsigned int> color_index_map{{rgba_purple, 7}, {rgba_lime, 42}, {rgba_grey, 88}};
//
//     ColorSet result = builder_.build(pal, color_index_map);
//
//     // Only indices 7 and 42 should be set
//     EXPECT_TRUE(result.test(7));
//     EXPECT_TRUE(result.test(42));
//     EXPECT_FALSE(result.test(88));
//     EXPECT_FALSE(result.test(0));
// }
//
// TEST_F(ColorSetBuilderTest, PanicsWhenColorNotInMap)
// {
//     NormalizedPal pal{rgba_black};
//     pal.insert(rgba_red);
//     pal.insert(rgba_green);
//
//     // Map doesn't contain rgba_green, should cause a panic
//     std::map<Rgba32, unsigned int> color_index_map{{rgba_red, 0}, {rgba_blue, 1}};
//
//     EXPECT_DEATH(
//         std::ignore = builder_.build(pal, color_index_map), "color_index_map did not contain requested color: 0 255
//         0");
// }
