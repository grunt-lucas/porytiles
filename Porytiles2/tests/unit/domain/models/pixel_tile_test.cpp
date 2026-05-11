#include <gtest/gtest.h>

#include <tuple>

#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"

using namespace porytiles2;

TEST(PixelTileTests, AtAndSet)
{
    PixelTile<IndexPixel> tile{};

    EXPECT_EQ(0, tile.at(0).index());
    EXPECT_EQ(0, tile.at(63).index());
    EXPECT_DEATH(std::ignore = tile.at(tile::size_pix), "index out of bounds: 64");
    EXPECT_DEATH(std::ignore = tile.at(tile::side_length_pix, 2), "row index out of bounds: 8");
    EXPECT_DEATH(std::ignore = tile.at(0, tile::side_length_pix), "col index out of bounds: 8");

    // Set value using index, fetch using row/col
    tile.set(22, 10);
    EXPECT_EQ(10, tile.at(2, 6).index());

    // Set value using row/col, fetch using index
    tile.set(5, 2, 31);
    EXPECT_EQ(31, tile.at(42).index());
}

TEST(PixelTileTests, IsTransparentExtrinsic)
{
    PixelTile<Rgba32> tile{};

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, Rgba32{255, 0, 255});
    }

    EXPECT_FALSE(tile.is_transparent(rgba_black));
    EXPECT_TRUE(tile.is_transparent(rgba_magenta));
}

TEST(PixelTileTests, IsTransparentAlpha)
{
    // Default-constructed PixelTile<Rgba32> is zeroed, i.e. black and intrinsically transparent
    const PixelTile<Rgba32> tile{};
    EXPECT_TRUE(tile.is_transparent(rgba_magenta));
}

TEST(PixelTileTests, IsTransparentMixed)
{
    PixelTile<Rgba32> tile{};

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, Rgba32{255, 0, 255});
    }

    // Set a pixel to non-magenta, but set alpha channel to transparent
    tile.set(12, Rgba32{22, 90, 144, Rgba32::alpha_transparent});

    EXPECT_FALSE(tile.is_transparent(rgba_black));
    EXPECT_TRUE(tile.is_transparent(rgba_magenta));
}

TEST(PixelTileTests, EqualsIgnoringIntrinsicTransparency)
{
    PixelTile<IndexPixel> tile1{};
    PixelTile<IndexPixel> tile2{};

    // Fill both tiles with same transparent value (index 0 is the only intrinsically transparent value for IndexPixel)
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile1.set(i, IndexPixel{0}); // transparent
        tile2.set(i, IndexPixel{0}); // same transparent value
    }

    EXPECT_TRUE(tile1.equals_ignoring_transparency(tile2));
    EXPECT_TRUE(tile2.equals_ignoring_transparency(tile1));
}

TEST(PixelTileTests, EqualsIgnoringDetectsNonTransparentDiff)
{
    PixelTile<IndexPixel> tile1{};
    PixelTile<IndexPixel> tile2{};

    // Fill both tiles with same non-transparent value
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile1.set(i, IndexPixel{5});
        tile2.set(i, IndexPixel{5});
    }

    // Change one pixel in tile2 to a different non-transparent value
    tile2.set(10, IndexPixel{6});

    EXPECT_FALSE(tile1.equals_ignoring_transparency(tile2));
    EXPECT_FALSE(tile2.equals_ignoring_transparency(tile1));
}

TEST(PixelTileTests, EqualsIgnoringMixedTransparency)
{
    PixelTile<IndexPixel> tile1{};
    PixelTile<IndexPixel> tile2{};

    // Fill both tiles with mix of transparent and non-transparent values
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        if (i % 2 == 0) {
            tile1.set(i, IndexPixel{0}); // transparent (index 0)
            tile2.set(i, IndexPixel{0}); // same transparent value
        }
        else {
            tile1.set(i, IndexPixel{5}); // non-transparent
            tile2.set(i, IndexPixel{5}); // same non-transparent value
        }
    }

    // Tiles should be equal since transparent pixels match logically and non-transparent match exactly
    EXPECT_TRUE(tile1.equals_ignoring_transparency(tile2));
    EXPECT_TRUE(tile2.equals_ignoring_transparency(tile1));
}

TEST(PixelTileTests, EqualsIgnoringExtrinsicTransparency)
{
    PixelTile<Rgba32> tile1{};
    PixelTile<Rgba32> tile2{};

    // Fill tile1 with extrinsically transparent pixels (magenta)
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile1.set(i, Rgba32{255, 0, 255});
    }

    // Fill tile2 with intrinsically transparent pixels (alpha=0)
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile2.set(i, Rgba32{0, 0, 0, Rgba32::alpha_transparent});
    }

    // With magenta as extrinsic transparency, both tiles should be considered equal
    EXPECT_TRUE(tile1.equals_ignoring_transparency(tile2, rgba_magenta));
    EXPECT_TRUE(tile2.equals_ignoring_transparency(tile1, rgba_magenta));
}

TEST(PixelTileTests, EqualsIgnoringDetectsExtrinsicDiff)
{
    PixelTile<Rgba32> tile1{};
    PixelTile<Rgba32> tile2{};

    // Fill both tiles with same non-transparent color
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile1.set(i, Rgba32{100, 150, 200});
        tile2.set(i, Rgba32{100, 150, 200});
    }

    // Change one pixel in tile2
    tile2.set(20, Rgba32{100, 150, 201});

    EXPECT_FALSE(tile1.equals_ignoring_transparency(tile2, rgba_magenta));
    EXPECT_FALSE(tile2.equals_ignoring_transparency(tile1, rgba_magenta));
}

TEST(PixelTileTests, EqualsIgnoringMixedExtrinsicTransparency)
{
    PixelTile<Rgba32> tile1{};
    PixelTile<Rgba32> tile2{};

    // Fill both tiles with mix of transparent and non-transparent
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        if (i % 3 == 0) {
            tile1.set(i, Rgba32{255, 0, 255});                        // extrinsically transparent (magenta)
            tile2.set(i, Rgba32{0, 0, 0, Rgba32::alpha_transparent}); // intrinsically transparent
        }
        else if (i % 3 == 1) {
            tile1.set(i, Rgba32{100, 150, 200}); // non-transparent
            tile2.set(i, Rgba32{100, 150, 200}); // same non-transparent
        }
        else {
            tile1.set(i, Rgba32{50, 75, 100, Rgba32::alpha_transparent}); // intrinsically transparent
            tile2.set(i, Rgba32{255, 0, 255});                            // extrinsically transparent (magenta)
        }
    }

    // With magenta as extrinsic transparency, tiles should be equal
    EXPECT_TRUE(tile1.equals_ignoring_transparency(tile2, rgba_magenta));
    EXPECT_TRUE(tile2.equals_ignoring_transparency(tile1, rgba_magenta));
}

TEST(PixelTileTests, EqualsIgnoringDistinctExtrinsicsBothTransparent)
{
    PixelTile<Rgba32> tile1{};
    PixelTile<Rgba32> tile2{};

    // tile1 is entirely magenta (extrinsically transparent under magenta).
    // tile2 is entirely cyan (extrinsically transparent under cyan).
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile1.set(i, rgba_magenta);
        tile2.set(i, rgba_cyan);
    }

    // Each tile is fully transparent under its own extrinsic, so the split-extrinsic
    // overload must consider them equal in both directions.
    EXPECT_TRUE(tile1.equals_ignoring_transparency(tile2, rgba_magenta, rgba_cyan));
    EXPECT_TRUE(tile2.equals_ignoring_transparency(tile1, rgba_cyan, rgba_magenta));

    // The single-extrinsic overload cannot reach the same conclusion: whichever extrinsic
    // is chosen, the other tile's pixels look opaque and fail the per-pixel comparison.
    EXPECT_FALSE(tile1.equals_ignoring_transparency(tile2, rgba_magenta));
    EXPECT_FALSE(tile1.equals_ignoring_transparency(tile2, rgba_cyan));
}

TEST(PixelTileTests, EqualsIgnoringDistinctExtrinsicsOpaqueIdentical)
{
    PixelTile<Rgba32> tile1{};
    PixelTile<Rgba32> tile2{};

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile1.set(i, Rgba32{100, 150, 200});
        tile2.set(i, Rgba32{100, 150, 200});
    }

    EXPECT_TRUE(tile1.equals_ignoring_transparency(tile2, rgba_magenta, rgba_cyan));
    EXPECT_TRUE(tile1.equals_ignoring_transparency(tile2, rgba_cyan, rgba_magenta));
}

TEST(PixelTileTests, EqualsIgnoringDistinctExtrinsicsDetectsOpaqueDiff)
{
    PixelTile<Rgba32> tile1{};
    PixelTile<Rgba32> tile2{};

    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile1.set(i, Rgba32{100, 150, 200});
        tile2.set(i, Rgba32{100, 150, 200});
    }

    tile2.set(20, Rgba32{100, 150, 201});

    EXPECT_FALSE(tile1.equals_ignoring_transparency(tile2, rgba_magenta, rgba_cyan));
    EXPECT_FALSE(tile2.equals_ignoring_transparency(tile1, rgba_cyan, rgba_magenta));
}

TEST(PixelTileTests, EqualsIgnoringDistinctExtrinsicsMixed)
{
    PixelTile<Rgba32> tile1{};
    PixelTile<Rgba32> tile2{};

    // Alternate between "each tile transparent under its own extrinsic" positions and
    // "identical opaque color" positions.
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        if (i % 2 == 0) {
            tile1.set(i, rgba_magenta);
            tile2.set(i, rgba_cyan);
        }
        else {
            tile1.set(i, rgba_yellow);
            tile2.set(i, rgba_yellow);
        }
    }

    EXPECT_TRUE(tile1.equals_ignoring_transparency(tile2, rgba_magenta, rgba_cyan));
    EXPECT_TRUE(tile2.equals_ignoring_transparency(tile1, rgba_cyan, rgba_magenta));
}

TEST(PixelTileTests, EqualsIgnoringDistinctExtrinsicsFalseWhenSingleTrue)
{
    PixelTile<Rgba32> tile1{};
    PixelTile<Rgba32> tile2{};

    // tile1 is intrinsically transparent everywhere (alpha=0).
    // tile2 is magenta everywhere (opaque alpha).
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile1.set(i, Rgba32{0, 0, 0, Rgba32::alpha_transparent});
        tile2.set(i, rgba_magenta);
    }

    // With a single magenta extrinsic, both tiles read as fully transparent -> equal.
    EXPECT_TRUE(tile1.equals_ignoring_transparency(tile2, rgba_magenta));

    // Split extrinsics force tile2's magenta pixels to be compared under cyan, where
    // they are no longer extrinsically transparent. The per-pixel comparison then
    // fails because alpha=0 black is not equal to opaque magenta.
    EXPECT_FALSE(tile1.equals_ignoring_transparency(tile2, rgba_magenta, rgba_cyan));
}

TEST(PixelTileTests, CrossEtCompareIsStrictWeakOrdering)
{
    // Reflexivity: compare(a, a) under a single ET must be equivalent.
    PixelTile<Rgba32> tile{};
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, Rgba32{100, 150, 200});
    }
    tile.set(0, rgba_magenta);
    EXPECT_EQ(
        PixelTile<Rgba32>::cross_et_compare(tile, rgba_magenta, tile, rgba_magenta), std::weak_ordering::equivalent);

    // Asymmetry: if a < b then b > a.
    PixelTile<Rgba32> opaque_lo{};
    PixelTile<Rgba32> opaque_hi{};
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        opaque_lo.set(i, Rgba32{10, 10, 10});
        opaque_hi.set(i, Rgba32{20, 20, 20});
    }
    const auto lo_vs_hi = PixelTile<Rgba32>::cross_et_compare(opaque_lo, rgba_magenta, opaque_hi, rgba_magenta);
    const auto hi_vs_lo = PixelTile<Rgba32>::cross_et_compare(opaque_hi, rgba_magenta, opaque_lo, rgba_magenta);
    EXPECT_EQ(lo_vs_hi, std::weak_ordering::less);
    EXPECT_EQ(hi_vs_lo, std::weak_ordering::greater);

    // Transitivity spot-check: a < b < c implies a < c.
    PixelTile<Rgba32> mid{};
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        mid.set(i, Rgba32{15, 15, 15});
    }
    EXPECT_EQ(
        PixelTile<Rgba32>::cross_et_compare(opaque_lo, rgba_magenta, mid, rgba_magenta), std::weak_ordering::less);
    EXPECT_EQ(
        PixelTile<Rgba32>::cross_et_compare(mid, rgba_magenta, opaque_hi, rgba_magenta), std::weak_ordering::less);
    EXPECT_EQ(
        PixelTile<Rgba32>::cross_et_compare(opaque_lo, rgba_magenta, opaque_hi, rgba_magenta),
        std::weak_ordering::less);
}

TEST(PixelTileTests, CrossEtCompareEquivalentUnderDifferentEts)
{
    // Two tiles with identical opaque-pixel patterns but different transparent colors
    // must compare equivalent under the split-ET relation.
    PixelTile<Rgba32> magenta_bg{};
    PixelTile<Rgba32> cyan_bg{};
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        magenta_bg.set(i, rgba_magenta);
        cyan_bg.set(i, rgba_cyan);
    }
    // Sprinkle identical opaque pixels at the same positions.
    magenta_bg.set(3, Rgba32{50, 100, 150});
    cyan_bg.set(3, Rgba32{50, 100, 150});
    magenta_bg.set(17, Rgba32{200, 10, 20});
    cyan_bg.set(17, Rgba32{200, 10, 20});

    EXPECT_EQ(
        PixelTile<Rgba32>::cross_et_compare(magenta_bg, rgba_magenta, cyan_bg, rgba_cyan),
        std::weak_ordering::equivalent);
    EXPECT_EQ(
        PixelTile<Rgba32>::cross_et_compare(cyan_bg, rgba_cyan, magenta_bg, rgba_magenta),
        std::weak_ordering::equivalent);

    // Sanity check against the equivalent equality relation.
    EXPECT_TRUE(magenta_bg.equals_ignoring_transparency(cyan_bg, rgba_magenta, rgba_cyan));
}

TEST(PixelTileTests, CrossEtCompareOrdersByOpaquePixels)
{
    // Two tiles that differ only in an opaque pixel compare in a consistent order, even though
    // their transparent backgrounds are different colors (each classified under its own ET).
    PixelTile<Rgba32> a{};
    PixelTile<Rgba32> b{};
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        a.set(i, rgba_magenta);
        b.set(i, rgba_cyan);
    }
    a.set(10, Rgba32{10, 0, 0});
    b.set(10, Rgba32{20, 0, 0});

    EXPECT_EQ(PixelTile<Rgba32>::cross_et_compare(a, rgba_magenta, b, rgba_cyan), std::weak_ordering::less);
    EXPECT_EQ(PixelTile<Rgba32>::cross_et_compare(b, rgba_cyan, a, rgba_magenta), std::weak_ordering::greater);
}

TEST(PixelTileTests, UniqueColorsEmptyForDefault)
{
    const PixelTile<IndexPixel> tile{};

    const auto colors = tile.unique_nontransparent_colors();

    // Default-constructed tile has all pixels set to IndexPixel{0} (transparent), so should return empty set
    EXPECT_EQ(0, colors.size());
}

TEST(PixelTileTests, UniqueColorsSingleForUniform)
{
    PixelTile<IndexPixel> tile{};

    // Fill entire tile with same non-transparent color
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, IndexPixel{7});
    }

    const auto colors = tile.unique_nontransparent_colors();

    EXPECT_EQ(1, colors.size());
    EXPECT_TRUE(colors.contains(IndexPixel{7}));
}

TEST(PixelTileTests, UniqueColorsMultipleForVaried)
{
    PixelTile<IndexPixel> tile{};

    // Fill tile with multiple non-transparent colors in a pattern
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        if (i % 4 == 0) {
            tile.set(i, IndexPixel{1});
        }
        else if (i % 4 == 1) {
            tile.set(i, IndexPixel{2});
        }
        else if (i % 4 == 2) {
            tile.set(i, IndexPixel{3});
        }
        else {
            tile.set(i, IndexPixel{4});
        }
    }

    const auto colors = tile.unique_nontransparent_colors();

    EXPECT_EQ(4, colors.size());
    EXPECT_TRUE(colors.contains(IndexPixel{1}));
    EXPECT_TRUE(colors.contains(IndexPixel{2}));
    EXPECT_TRUE(colors.contains(IndexPixel{3}));
    EXPECT_TRUE(colors.contains(IndexPixel{4}));
}

TEST(PixelTileTests, UniqueColorsWithRgba32)
{
    PixelTile<Rgba32> tile{};

    // Set some pixels to different non-transparent colors
    tile.set(0, Rgba32{255, 0, 0}); // red
    tile.set(1, Rgba32{0, 255, 0}); // green
    tile.set(2, Rgba32{0, 0, 255}); // blue
    tile.set(3, Rgba32{255, 0, 0}); // red again (duplicate)

    // Rest remain default (black with alpha=0, intrinsically transparent)

    const auto colors = tile.unique_nontransparent_colors(rgba_magenta);

    // Default black with alpha=0 is filtered out because it's intrinsically transparent
    EXPECT_EQ(3, colors.size());
    EXPECT_TRUE(colors.contains(Rgba32{255, 0, 0}));
    EXPECT_TRUE(colors.contains(Rgba32{0, 255, 0}));
    EXPECT_TRUE(colors.contains(Rgba32{0, 0, 255}));
}

TEST(PixelTileTests, UniqueColorsExcludesIntrinsicTransparent)
{
    PixelTile<IndexPixel> tile{};

    // Fill tile with mix of transparent (index 0) and non-transparent colors
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        if (i % 2 == 0) {
            tile.set(i, IndexPixel{0}); // intrinsically transparent
        }
        else {
            tile.set(i, IndexPixel{5}); // non-transparent
        }
    }

    const auto colors = tile.unique_nontransparent_colors();

    EXPECT_EQ(1, colors.size());
    EXPECT_TRUE(colors.contains(IndexPixel{5}));
    EXPECT_FALSE(colors.contains(IndexPixel{0}));
}

TEST(PixelTileTests, UniqueColorsExcludesAllIntrinsicRgba32)
{
    PixelTile<Rgba32> tile{};

    // Set pixels to different RGB values, all with alpha=0 (intrinsically transparent)
    tile.set(0, Rgba32{255, 0, 0, Rgba32::alpha_transparent});     // red, transparent
    tile.set(1, Rgba32{0, 255, 0, Rgba32::alpha_transparent});     // green, transparent
    tile.set(2, Rgba32{0, 0, 255, Rgba32::alpha_transparent});     // blue, transparent
    tile.set(3, Rgba32{255, 0, 0, Rgba32::alpha_transparent});     // red, transparent (duplicate)
    tile.set(4, Rgba32{100, 100, 100, Rgba32::alpha_transparent}); // gray, transparent

    // Rest remain default (black with alpha=0)

    const auto colors = tile.unique_nontransparent_colors(rgba_magenta);

    // Should return empty set because all pixels are intrinsically transparent
    EXPECT_EQ(0, colors.size());
}

TEST(PixelTileTests, UniqueColorsExcludesExtrinsicTransparent)
{
    PixelTile<Rgba32> tile{};

    // Set some pixels to extrinsically transparent color (magenta)
    tile.set(0, Rgba32{255, 0, 255});   // magenta (extrinsically transparent)
    tile.set(1, Rgba32{255, 0, 255});   // magenta (duplicate)
    tile.set(2, Rgba32{100, 150, 200}); // non-transparent color

    // Rest remain default (black with alpha=0, intrinsically transparent)

    const auto colors = tile.unique_nontransparent_colors(rgba_magenta);

    // Magenta is excluded (extrinsically transparent)
    // Default black is excluded (intrinsically transparent)
    EXPECT_EQ(1, colors.size());
    EXPECT_TRUE(colors.contains(Rgba32{100, 150, 200}));
    EXPECT_FALSE(colors.contains(Rgba32{255, 0, 255}));
    EXPECT_FALSE(colors.contains(Rgba32{}));
}

TEST(PixelTileTests, UniqueColorsExcludesBothTransparent)
{
    PixelTile<Rgba32> tile{};

    // Create a tile with various types of transparency and non-transparent colors
    tile.set(0, Rgba32{255, 0, 255});                          // extrinsically transparent (magenta)
    tile.set(1, Rgba32{100, 0, 0, Rgba32::alpha_transparent}); // intrinsically transparent (red with alpha=0)
    tile.set(2, Rgba32{0, 100, 0, Rgba32::alpha_transparent}); // intrinsically transparent (green with alpha=0)
    tile.set(3, Rgba32{200, 200, 200});                        // non-transparent (gray)
    tile.set(4, Rgba32{50, 50, 50});                           // non-transparent (dark gray)
    tile.set(5, Rgba32{255, 0, 255});                          // extrinsically transparent (magenta, duplicate)
    tile.set(
        6, Rgba32{100, 0, 0, Rgba32::alpha_transparent}); // intrinsically transparent (red with alpha=0, duplicate)

    // Rest remain default (black with alpha=0)

    const auto colors = tile.unique_nontransparent_colors(rgba_magenta);

    // Should only include the 2 non-transparent colors: gray and dark gray
    // Magenta is excluded (extrinsically transparent)
    // Red and green with alpha=0 are excluded (intrinsically transparent)
    // Default black is excluded (intrinsically transparent)
    EXPECT_EQ(2, colors.size());
    EXPECT_TRUE(colors.contains(Rgba32{200, 200, 200}));
    EXPECT_TRUE(colors.contains(Rgba32{50, 50, 50}));
    EXPECT_FALSE(colors.contains(Rgba32{255, 0, 255}));
    EXPECT_FALSE(colors.contains(Rgba32{100, 0, 0, Rgba32::alpha_transparent}));
    EXPECT_FALSE(colors.contains(Rgba32{0, 100, 0, Rgba32::alpha_transparent}));
    EXPECT_FALSE(colors.contains(Rgba32{}));
}
