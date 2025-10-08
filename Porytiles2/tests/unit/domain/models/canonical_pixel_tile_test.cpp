#include <gtest/gtest.h>

#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/canonical_pixel_tile.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"

using namespace porytiles2;

TEST(CanonicalPixelTileTests, ShouldFindCanonicalRepresentation)
{
    // Create a test tile with a distinctive pattern
    // Pattern:
    // 1 2 3 4 5 6 7 8
    // 2 3 4 5 6 7 8 9
    // 3 4 5 6 7 8 9 10
    // ... etc
    PixelTile<IndexPixel> tile{};
    for (std::size_t row = 0; row < 8; ++row) {
        for (std::size_t col = 0; col < 8; ++col) {
            tile.set(row, col, IndexPixel{static_cast<unsigned int>(row + col + 1)});
        }
    }

    // Create canonical tile
    CanonicalPixelTile<IndexPixel> canonical{tile};

    // The canonical representation should be one of the 4 flip combinations
    // We verify that:
    // 1. The canonical tile's pixels match one of the flip combinations
    // 2. The h_flip and v_flip flags are set correctly

    auto no_flip = tile.flip(false, false);
    auto h_flip = tile.flip(true, false);
    auto v_flip = tile.flip(false, true);
    auto hv_flip = tile.flip(true, true);

    // The canonical tile should equal one of these
    // Cast to PixelTile to avoid operator== ambiguity
    bool matches_no_flip = (static_cast<const PixelTile<IndexPixel> &>(canonical) == no_flip);
    bool matches_h_flip = (static_cast<const PixelTile<IndexPixel> &>(canonical) == h_flip);
    bool matches_v_flip = (static_cast<const PixelTile<IndexPixel> &>(canonical) == v_flip);
    bool matches_hv_flip = (static_cast<const PixelTile<IndexPixel> &>(canonical) == hv_flip);

    EXPECT_TRUE(matches_no_flip || matches_h_flip || matches_v_flip || matches_hv_flip);

    // Verify the flip flags match the actual representation
    auto expected_tile = tile.flip(canonical.h_flip(), canonical.v_flip());
    EXPECT_EQ(static_cast<const PixelTile<IndexPixel> &>(canonical), expected_tile);
}

TEST(CanonicalPixelTileTests, ShouldChooseMinimalRepresentation)
{
    // Create a tile where we know which flip combination should be minimal
    // We'll create a pattern where the no-flip version is lexicographically smallest
    PixelTile<IndexPixel> tile{};

    // Set top-left to smallest value
    tile.set(0, 0, IndexPixel{1});

    // Fill rest with larger values
    for (std::size_t i = 1; i < 64; ++i) {
        tile.set(i, IndexPixel{15});
    }

    CanonicalPixelTile<IndexPixel> canonical{tile};

    // The canonical representation should be the one with smallest value at (0,0)
    // which is the no-flip version
    EXPECT_EQ(IndexPixel{1}, canonical.at(0, 0));
}

TEST(CanonicalPixelTileTests, ShouldHandleSymmetricTiles)
{
    // Create a fully symmetric tile (all same values)
    PixelTile<IndexPixel> tile{};
    for (std::size_t i = 0; i < 64; ++i) {
        tile.set(i, IndexPixel{5});
    }

    CanonicalPixelTile<IndexPixel> canonical{tile};

    // For a symmetric tile, any flip combination should be equal
    EXPECT_EQ(static_cast<const PixelTile<IndexPixel> &>(canonical), tile);

    // The flip flags should be one of the valid combinations
    // Since all flips are equal, the algorithm should pick the lexicographically smallest
    // which would be (false, false)
    EXPECT_FALSE(canonical.h_flip());
    EXPECT_FALSE(canonical.v_flip());
}

TEST(CanonicalPixelTileTests, ShouldProduceConsistentResults)
{
    // Create a test tile
    PixelTile<IndexPixel> tile{};
    for (std::size_t i = 0; i < 64; ++i) {
        tile.set(i, IndexPixel{static_cast<unsigned int>(i % 16)});
    }

    // Create multiple canonical tiles from the same source
    CanonicalPixelTile<IndexPixel> canonical1{tile};
    CanonicalPixelTile<IndexPixel> canonical2{tile};

    // They should be identical
    EXPECT_EQ(canonical1, canonical2);
    EXPECT_EQ(canonical1.h_flip(), canonical2.h_flip());
    EXPECT_EQ(canonical1.v_flip(), canonical2.v_flip());
}

TEST(CanonicalPixelTileTests, ShouldHandleAllFlipVariations)
{
    // Create a test tile
    PixelTile<IndexPixel> tile{};
    tile.set(0, 0, IndexPixel{1});
    tile.set(0, 7, IndexPixel{2});
    tile.set(7, 0, IndexPixel{3});
    tile.set(7, 7, IndexPixel{4});

    // Test that creating canonical tiles from different flips
    // of the same logical tile gives consistent results
    auto no_flip = tile.flip(false, false);
    auto h_flip = tile.flip(true, false);
    auto v_flip = tile.flip(false, true);
    auto hv_flip = tile.flip(true, true);

    CanonicalPixelTile<IndexPixel> canonical_from_no_flip{no_flip};
    CanonicalPixelTile<IndexPixel> canonical_from_h_flip{h_flip};
    CanonicalPixelTile<IndexPixel> canonical_from_v_flip{v_flip};
    CanonicalPixelTile<IndexPixel> canonical_from_hv_flip{hv_flip};

    // All canonical representations should have the same pixel data
    // (though their flip flags may differ)
    EXPECT_EQ(
        static_cast<const PixelTile<IndexPixel> &>(canonical_from_no_flip),
        static_cast<const PixelTile<IndexPixel> &>(canonical_from_h_flip));
    EXPECT_EQ(
        static_cast<const PixelTile<IndexPixel> &>(canonical_from_no_flip),
        static_cast<const PixelTile<IndexPixel> &>(canonical_from_v_flip));
    EXPECT_EQ(
        static_cast<const PixelTile<IndexPixel> &>(canonical_from_no_flip),
        static_cast<const PixelTile<IndexPixel> &>(canonical_from_hv_flip));
}

TEST(CanonicalPixelTileTests, ShouldWorkWithRgba32Pixels)
{
    // Create a test tile with Rgba32 colors
    // Use distinctive colors at each corner
    PixelTile<Rgba32> tile{};

    // Fill with a base color
    for (std::size_t i = 0; i < 64; ++i) {
        tile.set(i, rgba_white);
    }

    // Set distinctive colors at corners
    tile.set(0, 0, rgba_red);    // Top-left
    tile.set(0, 7, rgba_green);  // Top-right
    tile.set(7, 0, rgba_blue);   // Bottom-left
    tile.set(7, 7, rgba_yellow); // Bottom-right

    // Create canonical tile
    CanonicalPixelTile<Rgba32> canonical{tile};

    // Verify that the canonical representation matches one of the flip combinations
    auto no_flip = tile.flip(false, false);
    auto h_flip = tile.flip(true, false);
    auto v_flip = tile.flip(false, true);
    auto hv_flip = tile.flip(true, true);

    bool matches_no_flip = (static_cast<const PixelTile<Rgba32> &>(canonical) == no_flip);
    bool matches_h_flip = (static_cast<const PixelTile<Rgba32> &>(canonical) == h_flip);
    bool matches_v_flip = (static_cast<const PixelTile<Rgba32> &>(canonical) == v_flip);
    bool matches_hv_flip = (static_cast<const PixelTile<Rgba32> &>(canonical) == hv_flip);

    EXPECT_TRUE(matches_no_flip || matches_h_flip || matches_v_flip || matches_hv_flip);

    // Verify the flip flags correctly reconstruct the canonical form
    auto expected_tile = tile.flip(canonical.h_flip(), canonical.v_flip());
    EXPECT_EQ(static_cast<const PixelTile<Rgba32> &>(canonical), expected_tile);

    // Test that all flipped versions produce the same canonical representation
    CanonicalPixelTile<Rgba32> canonical_from_h{h_flip};
    CanonicalPixelTile<Rgba32> canonical_from_v{v_flip};
    CanonicalPixelTile<Rgba32> canonical_from_hv{hv_flip};

    // All should have the same pixel data in their canonical form
    EXPECT_EQ(
        static_cast<const PixelTile<Rgba32> &>(canonical), static_cast<const PixelTile<Rgba32> &>(canonical_from_h));
    EXPECT_EQ(
        static_cast<const PixelTile<Rgba32> &>(canonical), static_cast<const PixelTile<Rgba32> &>(canonical_from_v));
    EXPECT_EQ(
        static_cast<const PixelTile<Rgba32> &>(canonical), static_cast<const PixelTile<Rgba32> &>(canonical_from_hv));
}
