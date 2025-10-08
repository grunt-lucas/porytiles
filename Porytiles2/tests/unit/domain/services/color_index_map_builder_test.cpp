#include "gtest/gtest.h"

#include <ranges>
#include <vector>

#include "porytiles2/domain/services/color_index_map_builder.hpp"
#include "porytiles2/xcut/panic/panic.hpp"

using namespace porytiles2;

// class ColorIndexMapBuilderTests : public ::testing::Test {
//   protected:
//     void SetUp() override
//     {
//         builder_ = std::make_unique<ColorIndexMapBuilder>();
//     }
//
//     static NormalizedTile<Rgba32> create_tile_with_colors(const std::vector<Rgba32> &colors, const Rgba32 &extrinsic)
//     {
//         NormalizedTile tile{false, false, extrinsic};
//
//         // Insert all colors into the palette
//         for (const auto &color : colors) {
//             tile.palette().insert(color);
//         }
//
//         // Set tile pixels to reference each palette color at least once
//         // IndexPixel 0 = transparent, IndexPixel 1+ = palette colors (1-based indexing)
//         std::size_t pixel_index = 0;
//         for (std::size_t color_idx = 0; color_idx < colors.size() && pixel_index < 64; ++color_idx) {
//             // Set multiple pixels to this palette color for better test coverage
//             const IndexPixel palette_index{static_cast<unsigned int>(color_idx + 1)}; // 1-based indexing
//             for (int rep = 0; rep < 3 && pixel_index < 64; ++rep, ++pixel_index) {
//                 tile.set(pixel_index, palette_index);
//             }
//         }
//
//         // Fill remaining pixels with the first palette color (or transparent if no colors)
//         const IndexPixel fill_value = colors.empty() ? IndexPixel{0} : IndexPixel{1};
//         for (; pixel_index < 64; ++pixel_index) {
//             tile.set(pixel_index, fill_value);
//         }
//
//         return tile;
//     }
//
//     std::unique_ptr<ColorIndexMapBuilder> builder_;
// };
//
// TEST_F(ColorIndexMapBuilderTests, EmptyTilesProducesEmptyMap)
// {
//     std::vector<NormalizedTile<Rgba32>> empty_tiles{};
//     const Rgba32 extrinsic{255, 0, 255}; // Magenta as transparency
//
//     const auto result = builder_->build_map(empty_tiles, extrinsic);
//
//     EXPECT_TRUE(result.empty());
// }
//
// TEST_F(ColorIndexMapBuilderTests, SingleTileWithOneColorProducesCorrectMapping)
// {
//     const Rgba32 extrinsic{255, 0, 255}; // Magenta as transparency
//     const Rgba32 red{255, 0, 0};
//
//     std::vector<NormalizedTile<Rgba32>> tiles{};
//     tiles.push_back(create_tile_with_colors({red}, extrinsic));
//
//     const auto result = builder_->build_map(tiles, extrinsic);
//
//     EXPECT_EQ(result.size(), 1);
//     EXPECT_EQ(result.at(red), 0);
// }
//
// TEST_F(ColorIndexMapBuilderTests, MultipleColorsAssignedSequentialIndices)
// {
//     const Rgba32 extrinsic{255, 0, 255}; // Magenta as transparency
//     const Rgba32 red{255, 0, 0};
//     const Rgba32 green{0, 255, 0};
//     const Rgba32 blue{0, 0, 255};
//
//     std::vector<NormalizedTile<Rgba32>> tiles{};
//     tiles.push_back(create_tile_with_colors({red, green, blue}, extrinsic));
//
//     const auto result = builder_->build_map(tiles, extrinsic);
//
//     EXPECT_EQ(result.size(), 3);
//     // Colors should be assigned indices based on their insertion order
//     EXPECT_TRUE(result.contains(red));
//     EXPECT_TRUE(result.contains(green));
//     EXPECT_TRUE(result.contains(blue));
//
//     // Verify indices are unique and in the range [0, 2]
//     std::set<unsigned int> assigned_indices{};
//     for (const auto &index : result | std::views::values) {
//         assigned_indices.insert(index);
//         EXPECT_LT(index, 3);
//     }
//     EXPECT_EQ(assigned_indices.size(), 3);
// }
//
// TEST_F(ColorIndexMapBuilderTests, MultipleTilesWithDuplicateColorsAssignSameIndex)
// {
//     const Rgba32 extrinsic{255, 0, 255}; // Magenta as transparency
//     const Rgba32 red{255, 0, 0};
//     const Rgba32 green{0, 255, 0};
//
//     std::vector<NormalizedTile<Rgba32>> tiles{};
//     tiles.push_back(create_tile_with_colors({red}, extrinsic));
//     tiles.push_back(create_tile_with_colors({green, red}, extrinsic)); // red appears in both tiles
//
//     const auto result = builder_->build_map(tiles, extrinsic);
//
//     EXPECT_EQ(result.size(), 2);
//     EXPECT_TRUE(result.contains(red));
//     EXPECT_TRUE(result.contains(green));
//
//     // Verify both colors get unique indices
//     EXPECT_NE(result.at(red), result.at(green));
// }
//
// TEST_F(ColorIndexMapBuilderTests, ColorsFromMultipleTilesCombinedCorrectly)
// {
//     const Rgba32 extrinsic{255, 0, 255}; // Magenta as transparency
//     const Rgba32 red{255, 0, 0};
//     const Rgba32 green{0, 255, 0};
//     const Rgba32 blue{0, 0, 255};
//     const Rgba32 yellow{255, 255, 0};
//
//     std::vector<NormalizedTile<Rgba32>> tiles{};
//     tiles.push_back(create_tile_with_colors({red, green}, extrinsic));
//     tiles.push_back(create_tile_with_colors({blue, yellow}, extrinsic));
//     tiles.push_back(create_tile_with_colors({red, blue}, extrinsic)); // Overlapping colors
//
//     const auto result = builder_->build_map(tiles, extrinsic);
//
//     EXPECT_EQ(result.size(), 4);
//     EXPECT_TRUE(result.contains(red));
//     EXPECT_TRUE(result.contains(green));
//     EXPECT_TRUE(result.contains(blue));
//     EXPECT_TRUE(result.contains(yellow));
//
//     // Verify all indices are unique
//     std::set<unsigned int> assigned_indices{};
//     for (const auto &index : result | std::views::values) {
//         assigned_indices.insert(index);
//     }
//     EXPECT_EQ(assigned_indices.size(), 4);
// }
//
// TEST_F(ColorIndexMapBuilderTests, EmptyPaletteTilesHandledCorrectly)
// {
//     const Rgba32 extrinsic{255, 0, 255}; // Magenta as transparency
//
//     std::vector<NormalizedTile<Rgba32>> tiles{};
//     tiles.push_back(create_tile_with_colors({}, extrinsic)); // Empty palette
//
//     const auto result = builder_->build_map(tiles, extrinsic);
//
//     EXPECT_TRUE(result.empty());
// }
//
// TEST_F(ColorIndexMapBuilderTests, MixOfEmptyAndNonEmptyTilesHandledCorrectly)
// {
//     const Rgba32 extrinsic{255, 0, 255}; // Magenta as transparency
//     const Rgba32 red{255, 0, 0};
//     const Rgba32 green{0, 255, 0};
//
//     std::vector<NormalizedTile<Rgba32>> tiles{};
//     tiles.push_back(create_tile_with_colors({}, extrinsic)); // Empty palette
//     tiles.push_back(create_tile_with_colors({red, green}, extrinsic));
//     tiles.push_back(create_tile_with_colors({}, extrinsic)); // Another empty palette
//
//     const auto result = builder_->build_map(tiles, extrinsic);
//
//     EXPECT_EQ(result.size(), 2);
//     EXPECT_TRUE(result.contains(red));
//     EXPECT_TRUE(result.contains(green));
// }
//
// // Death tests to verify panic conditions
// TEST_F(ColorIndexMapBuilderTests, ShouldPanicOnNonOpaqueColor)
// {
//     constexpr Rgba32 extrinsic{255, 0, 255};           // Magenta as transparency
//     constexpr Rgba32 semi_transparent{255, 0, 0, 128}; // Red with alpha=128 (not fully opaque)
//
//     std::vector<NormalizedTile<Rgba32>> tiles{};
//     tiles.push_back(create_tile_with_colors({semi_transparent}, extrinsic));
//
//     ASSERT_DEATH(std::ignore = builder_->build_map(tiles, extrinsic), "invalid rgba");
// }
//
// TEST_F(ColorIndexMapBuilderTests, ShouldPanicOnTransparentColor)
// {
//     constexpr Rgba32 extrinsic{255, 0, 255};          // Magenta as transparency
//     constexpr Rgba32 transparent_color{255, 0, 0, 0}; // Fully transparent red
//
//     std::vector<NormalizedTile<Rgba32>> tiles{};
//     tiles.push_back(create_tile_with_colors({transparent_color}, extrinsic));
//
//     ASSERT_DEATH(std::ignore = builder_->build_map(tiles, extrinsic), "invalid rgba");
// }
//
// TEST_F(ColorIndexMapBuilderTests, ShouldPanicOnExtrinsicTransparencyColor)
// {
//     constexpr Rgba32 extrinsic{255, 0, 255}; // Magenta as transparency
//
//     std::vector<NormalizedTile<Rgba32>> tiles{};
//     tiles.push_back(create_tile_with_colors({extrinsic}, extrinsic));
//
//     ASSERT_DEATH(std::ignore = builder_->build_map(tiles, extrinsic), "invalid rgba");
// }
//
// TEST_F(ColorIndexMapBuilderTests, ShouldNotPanicOnValidOpaqueColors)
// {
//     constexpr Rgba32 extrinsic{255, 0, 255};       // Magenta as transparency
//     constexpr Rgba32 opaque_red{255, 0, 0, 255};   // Fully opaque red
//     constexpr Rgba32 opaque_green{0, 255, 0, 255}; // Fully opaque green
//
//     std::vector<NormalizedTile<Rgba32>> tiles{};
//     tiles.push_back(create_tile_with_colors({opaque_red, opaque_green}, extrinsic));
//
//     // This should not panic - it should succeed normally
//     const auto result = builder_->build_map(tiles, extrinsic);
//     EXPECT_EQ(result.size(), 2);
//     EXPECT_TRUE(result.contains(opaque_red));
//     EXPECT_TRUE(result.contains(opaque_green));
// }
