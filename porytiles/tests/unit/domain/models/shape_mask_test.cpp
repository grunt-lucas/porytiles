#include "gtest/gtest.h"

#include <unordered_set>

#include "porytiles/domain/models/shape_mask.hpp"

using namespace porytiles;

namespace {
ShapeMask create_test_mask()
{
    // Creates a simple pattern:
    // 10000000  (0x80)
    // 01000000  (0x40)
    // 00100000  (0x20)
    // 00010000  (0x10)
    // 00001000  (0x08)
    // 00000100  (0x04)
    // 00000010  (0x02)
    // 00000001  (0x01)
    return ShapeMask{{0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01}};
}
} // namespace

TEST(ShapeMaskTests, DefaultConstruction)
{
    ShapeMask tm;
    EXPECT_EQ(tm.rows().size(), 8);
    for (const auto &row : tm.rows()) {
        EXPECT_EQ(row, 0);
    }
}

TEST(ShapeMaskTests, RowsAccessor)
{
    ShapeMask tm = create_test_mask();
    const auto &rows = tm.rows();
    EXPECT_EQ(rows[0], 0x80);
    EXPECT_EQ(rows[1], 0x40);
    EXPECT_EQ(rows[2], 0x20);
    EXPECT_EQ(rows[3], 0x10);
    EXPECT_EQ(rows[4], 0x08);
    EXPECT_EQ(rows[5], 0x04);
    EXPECT_EQ(rows[6], 0x02);
    EXPECT_EQ(rows[7], 0x01);
}

TEST(ShapeMaskTests, EqualityComparison)
{
    ShapeMask tm1 = create_test_mask();
    ShapeMask tm2 = create_test_mask();
    ShapeMask tm3;

    EXPECT_EQ(tm1, tm2);
    EXPECT_NE(tm1, tm3);
}

TEST(ShapeMaskTests, ComparisonOperators)
{
    ShapeMask tm1{{0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    ShapeMask tm2{{0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

    EXPECT_LT(tm1, tm2);
    EXPECT_LE(tm1, tm2);
    EXPECT_GT(tm2, tm1);
    EXPECT_GE(tm2, tm1);
}

TEST(ShapeMaskTests, GetFlipNoFlip)
{
    ShapeMask tm = create_test_mask();
    ShapeMask flipped = tm.flip(false, false);

    EXPECT_EQ(tm, flipped);
}

TEST(ShapeMaskTests, GetFlipHorizontal)
{
    ShapeMask tm = create_test_mask();
    ShapeMask flipped = tm.flip(true, false);

    // Horizontal flip reverses the bits in each row
    // 10000000 -> 00000001
    // 01000000 -> 00000010
    // 00100000 -> 00000100
    // etc.
    EXPECT_EQ(flipped.rows()[0], 0x01);
    EXPECT_EQ(flipped.rows()[1], 0x02);
    EXPECT_EQ(flipped.rows()[2], 0x04);
    EXPECT_EQ(flipped.rows()[3], 0x08);
    EXPECT_EQ(flipped.rows()[4], 0x10);
    EXPECT_EQ(flipped.rows()[5], 0x20);
    EXPECT_EQ(flipped.rows()[6], 0x40);
    EXPECT_EQ(flipped.rows()[7], 0x80);
}

TEST(ShapeMaskTests, GetFlipVertical)
{
    ShapeMask tm = create_test_mask();
    ShapeMask flipped = tm.flip(false, true);

    // Vertical flip reverses the order of rows
    EXPECT_EQ(flipped.rows()[0], 0x01);
    EXPECT_EQ(flipped.rows()[1], 0x02);
    EXPECT_EQ(flipped.rows()[2], 0x04);
    EXPECT_EQ(flipped.rows()[3], 0x08);
    EXPECT_EQ(flipped.rows()[4], 0x10);
    EXPECT_EQ(flipped.rows()[5], 0x20);
    EXPECT_EQ(flipped.rows()[6], 0x40);
    EXPECT_EQ(flipped.rows()[7], 0x80);
}

TEST(ShapeMaskTests, GetFlipBoth)
{
    ShapeMask tm = create_test_mask();
    ShapeMask flipped = tm.flip(true, true);

    // Both flips: rows are reversed AND bits in each row are reversed
    // Original row 7 (00000001) -> reversed bits (10000000) -> goes to row 0
    EXPECT_EQ(flipped.rows()[0], 0x80);
    EXPECT_EQ(flipped.rows()[1], 0x40);
    EXPECT_EQ(flipped.rows()[2], 0x20);
    EXPECT_EQ(flipped.rows()[3], 0x10);
    EXPECT_EQ(flipped.rows()[4], 0x08);
    EXPECT_EQ(flipped.rows()[5], 0x04);
    EXPECT_EQ(flipped.rows()[6], 0x02);
    EXPECT_EQ(flipped.rows()[7], 0x01);
}

TEST(ShapeMaskTests, GetFlipSymmetry)
{
    ShapeMask tm = create_test_mask();

    // Flipping twice should return to original
    ShapeMask h_flip = tm.flip(true, false);
    ShapeMask h_flip_back = h_flip.flip(true, false);
    EXPECT_EQ(tm, h_flip_back);

    ShapeMask v_flip = tm.flip(false, true);
    ShapeMask v_flip_back = v_flip.flip(false, true);
    EXPECT_EQ(tm, v_flip_back);

    ShapeMask both_flip = tm.flip(true, true);
    ShapeMask both_flip_back = both_flip.flip(true, true);
    EXPECT_EQ(tm, both_flip_back);
}

TEST(ShapeMaskTests, HashFunction)
{
    ShapeMask tm1 = create_test_mask();
    ShapeMask tm2 = create_test_mask();
    ShapeMask tm3;

    std::hash<ShapeMask> hasher;

    // Equal objects should have equal hashes
    EXPECT_EQ(hasher(tm1), hasher(tm2));

    // Different objects should (probably) have different hashes
    EXPECT_NE(hasher(tm1), hasher(tm3));
}

TEST(ShapeMaskTests, HashInUnorderedSet)
{
    std::unordered_set<ShapeMask> mask_set;

    ShapeMask tm1 = create_test_mask();
    ShapeMask tm2 = create_test_mask();
    ShapeMask tm3;

    mask_set.insert(tm1);
    mask_set.insert(tm2); // Should not be inserted (duplicate)
    mask_set.insert(tm3);

    EXPECT_EQ(mask_set.size(), 2);
    EXPECT_TRUE(mask_set.contains(tm1));
    EXPECT_TRUE(mask_set.contains(tm3));
}

TEST(ShapeMaskTests, SetBit)
{
    ShapeMask tm;

    // Set bit at row 0, col 0 (leftmost bit of first row)
    tm.set(0, 0);
    EXPECT_EQ(tm.rows()[0], 0x80); // 10000000

    // Set bit at row 0, col 7 (rightmost bit of first row)
    tm.set(0, 7);
    EXPECT_EQ(tm.rows()[0], 0x81); // 10000001

    // Set bit at row 3, col 4 (middle bit)
    tm.set(3, 4);
    EXPECT_EQ(tm.rows()[3], 0x08); // 00001000
}

TEST(ShapeMaskTests, UnsetBit)
{
    ShapeMask tm{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

    // Unset bit at row 0, col 0
    tm.unset(0, 0);
    EXPECT_EQ(tm.rows()[0], 0x7F); // 01111111

    // Unset bit at row 0, col 7
    tm.unset(0, 7);
    EXPECT_EQ(tm.rows()[0], 0x7E); // 01111110

    // Unset bit at row 5, col 3
    tm.unset(5, 3);
    EXPECT_EQ(tm.rows()[5], 0xEF); // 11101111
}

TEST(ShapeMaskTests, SetMultipleBits)
{
    ShapeMask tm;

    // Create a diagonal pattern
    for (int i = 0; i < 8; ++i) {
        tm.set(i, i);
    }

    // Verify diagonal pattern
    EXPECT_EQ(tm.rows()[0], 0x80); // 10000000
    EXPECT_EQ(tm.rows()[1], 0x40); // 01000000
    EXPECT_EQ(tm.rows()[2], 0x20); // 00100000
    EXPECT_EQ(tm.rows()[3], 0x10); // 00010000
    EXPECT_EQ(tm.rows()[4], 0x08); // 00001000
    EXPECT_EQ(tm.rows()[5], 0x04); // 00000100
    EXPECT_EQ(tm.rows()[6], 0x02); // 00000010
    EXPECT_EQ(tm.rows()[7], 0x01); // 00000001
}

TEST(ShapeMaskTests, SetIsIdempotent)
{
    ShapeMask tm;

    // Set a bit multiple times
    tm.set(2, 3);
    tm.set(2, 3);
    tm.set(2, 3);

    // Should only be set once
    EXPECT_EQ(tm.rows()[2], 0x10); // 00010000
}

TEST(ShapeMaskTests, UnsetIsIdempotent)
{
    ShapeMask tm{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

    // Unset a bit multiple times
    tm.unset(4, 5);
    tm.unset(4, 5);
    tm.unset(4, 5);

    // Should only be unset once
    EXPECT_EQ(tm.rows()[4], 0xFB); // 11111011
}

TEST(ShapeMaskTests, SetAndUnset)
{
    ShapeMask tm;

    // Set some bits
    tm.set(0, 0);
    tm.set(0, 7);
    tm.set(3, 3);

    EXPECT_EQ(tm.rows()[0], 0x81); // 10000001
    EXPECT_EQ(tm.rows()[3], 0x10); // 00010000

    // Unset one of them
    tm.unset(0, 0);

    EXPECT_EQ(tm.rows()[0], 0x01); // 00000001
    EXPECT_EQ(tm.rows()[3], 0x10); // 00010000 (unchanged)
}

TEST(ShapeMaskTests, IsTransparentForDefaultMask)
{
    ShapeMask tm;

    EXPECT_TRUE(tm.is_transparent());
}

TEST(ShapeMaskTests, IsTransparentForNonEmptyMask)
{
    ShapeMask tm = create_test_mask();

    EXPECT_FALSE(tm.is_transparent());
}

TEST(ShapeMaskTests, IsTransparentAfterSettingAndUnsetting)
{
    ShapeMask tm;

    // Start transparent
    EXPECT_TRUE(tm.is_transparent());

    // Set a bit - no longer transparent
    tm.set(3, 4);
    EXPECT_FALSE(tm.is_transparent());

    // Unset the bit - transparent again
    tm.unset(3, 4);
    EXPECT_TRUE(tm.is_transparent());
}
