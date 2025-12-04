#include "porytiles2/domain/packing/models/palette_pool.hpp"

#include <bitset>

#include "gtest/gtest.h"

using namespace porytiles2;

// =============================================================================
// Constructor Tests
// =============================================================================

TEST(PalettePoolConstructorTests, ConstructsWithAvailableIndexes)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);
    available.set(5);
    available.set(10);

    PalettePool pool{available};

    EXPECT_TRUE(pool.has_available_index());
}

TEST(PalettePoolConstructorTests, ConstructsWithNoAvailableIndexes)
{
    std::bitset<pal::num_pals> available{};

    PalettePool pool{available};

    EXPECT_FALSE(pool.has_available_index());
}

TEST(PalettePoolConstructorTests, ConstructsWithAllIndexesAvailable)
{
    std::bitset<pal::num_pals> available{};
    available.set(); // Set all bits

    PalettePool pool{available};

    EXPECT_TRUE(pool.has_available_index());
}

// =============================================================================
// has_available_index Tests
// =============================================================================

TEST(PalettePoolHasAvailableTests, ReturnsTrueWhenIndexesAvailable)
{
    std::bitset<pal::num_pals> available{};
    available.set(3);

    PalettePool pool{available};

    EXPECT_TRUE(pool.has_available_index());
}

TEST(PalettePoolHasAvailableTests, ReturnsFalseWhenAllCheckedOut)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);

    PalettePool pool{available};

    std::ignore = pool.check_out();

    EXPECT_FALSE(pool.has_available_index());
}

TEST(PalettePoolHasAvailableTests, ReturnsTrueAfterCheckIn)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);

    PalettePool pool{available};

    std::ignore = pool.check_out();
    EXPECT_FALSE(pool.has_available_index());

    pool.check_in();
    EXPECT_TRUE(pool.has_available_index());
}

// =============================================================================
// is_available(index) Tests
// =============================================================================

TEST(PalettePoolIsAvailableTests, ReturnsTrueForAvailableIndex)
{
    std::bitset<pal::num_pals> available{};
    available.set(3);
    available.set(7);

    PalettePool pool{available};

    EXPECT_TRUE(pool.is_available(3));
    EXPECT_TRUE(pool.is_available(7));
}

TEST(PalettePoolIsAvailableTests, ReturnsFalseForUnavailableIndex)
{
    std::bitset<pal::num_pals> available{};
    available.set(3);

    PalettePool pool{available};

    EXPECT_FALSE(pool.is_available(0));
    EXPECT_FALSE(pool.is_available(5));
    EXPECT_FALSE(pool.is_available(15));
}

TEST(PalettePoolIsAvailableTests, ReturnsFalseForCheckedOutIndex)
{
    std::bitset<pal::num_pals> available{};
    available.set(3);
    available.set(7);

    PalettePool pool{available};

    std::ignore = pool.check_out(); // checks out 3
    EXPECT_FALSE(pool.is_available(3));
    EXPECT_TRUE(pool.is_available(7));
}

TEST(PalettePoolIsAvailableTests, ReturnsTrueAfterCheckIn)
{
    std::bitset<pal::num_pals> available{};
    available.set(5);

    PalettePool pool{available};

    std::ignore = pool.check_out();
    EXPECT_FALSE(pool.is_available(5));

    pool.check_in();
    EXPECT_TRUE(pool.is_available(5));
}

TEST(PalettePoolIsAvailableTests, PanicsForOutOfBoundsIndex)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);

    PalettePool pool{available};

    EXPECT_DEATH(std::ignore = pool.is_available(16), "out of bounds");
    EXPECT_DEATH(std::ignore = pool.is_available(100), "out of bounds");
}

// =============================================================================
// check_out Tests
// =============================================================================

TEST(PalettePoolCheckOutTests, ReturnsLowestAvailableIndex)
{
    std::bitset<pal::num_pals> available{};
    available.set(5);
    available.set(10);
    available.set(15);

    PalettePool pool{available};

    EXPECT_EQ(pool.check_out(), 5);
}

TEST(PalettePoolCheckOutTests, ReturnsNextLowestAfterFirstCheckout)
{
    std::bitset<pal::num_pals> available{};
    available.set(2);
    available.set(7);
    available.set(12);

    PalettePool pool{available};

    EXPECT_EQ(pool.check_out(), 2);
    EXPECT_EQ(pool.check_out(), 7);
    EXPECT_EQ(pool.check_out(), 12);
}

TEST(PalettePoolCheckOutTests, ChecksOutAllAvailableIndexes)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);
    available.set(1);
    available.set(2);

    PalettePool pool{available};

    std::ignore = pool.check_out();
    std::ignore = pool.check_out();
    std::ignore = pool.check_out();

    EXPECT_FALSE(pool.has_available_index());
}

TEST(PalettePoolCheckOutTests, OnlyChecksOutAvailableIndexes)
{
    std::bitset<pal::num_pals> available{};
    // Only indexes 3, 7, 11 are available (not 0, 1, 2, etc.)
    available.set(3);
    available.set(7);
    available.set(11);

    PalettePool pool{available};

    // Should skip 0, 1, 2 and return 3
    EXPECT_EQ(pool.check_out(), 3);
    // Should skip 4, 5, 6 and return 7
    EXPECT_EQ(pool.check_out(), 7);
    // Should skip 8, 9, 10 and return 11
    EXPECT_EQ(pool.check_out(), 11);
}

TEST(PalettePoolCheckOutTests, PanicsWhenNoAvailableIndexes)
{
    std::bitset<pal::num_pals> available{};

    PalettePool pool{available};

    EXPECT_DEATH(std::ignore = pool.check_out(), "no available indexes");
}

TEST(PalettePoolCheckOutTests, PanicsWhenAllCheckedOut)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);

    PalettePool pool{available};

    std::ignore = pool.check_out();

    EXPECT_DEATH(std::ignore = pool.check_out(), "no available indexes");
}

// =============================================================================
// check_out(index) Tests
// =============================================================================

TEST(PalettePoolCheckOutIndexTests, ChecksOutSpecificIndex)
{
    std::bitset<pal::num_pals> available{};
    available.set(3);
    available.set(7);
    available.set(10);

    PalettePool pool{available};

    // Check out index 7 (not the lowest)
    pool.check_out(7);
    EXPECT_FALSE(pool.is_available(7));
    EXPECT_TRUE(pool.is_available(3));
    EXPECT_TRUE(pool.is_available(10));
}

TEST(PalettePoolCheckOutIndexTests, AddsToCheckoutStack)
{
    std::bitset<pal::num_pals> available{};
    available.set(3);
    available.set(7);

    PalettePool pool{available};

    // Check out specific indexes in non-sequential order
    pool.check_out(7);
    pool.check_out(3);

    // Check in follows LIFO - should return 3 first
    pool.check_in();
    EXPECT_TRUE(pool.is_available(3));
    EXPECT_FALSE(pool.is_available(7));

    pool.check_in();
    EXPECT_TRUE(pool.is_available(7));
}

TEST(PalettePoolCheckOutIndexTests, PanicsForUnavailableIndex)
{
    std::bitset<pal::num_pals> available{};
    available.set(3);

    PalettePool pool{available};

    EXPECT_DEATH(pool.check_out(5), "not available");
}

TEST(PalettePoolCheckOutIndexTests, PanicsForAlreadyCheckedOutIndex)
{
    std::bitset<pal::num_pals> available{};
    available.set(3);

    PalettePool pool{available};

    pool.check_out(3);

    EXPECT_DEATH(pool.check_out(3), "not available");
}

TEST(PalettePoolCheckOutIndexTests, PanicsForOutOfBoundsIndex)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);

    PalettePool pool{available};

    EXPECT_DEATH(pool.check_out(16), "out of bounds");
    EXPECT_DEATH(pool.check_out(100), "out of bounds");
}

TEST(PalettePoolCheckOutIndexTests, MixedAutoAndIndexedCheckouts)
{
    std::bitset<pal::num_pals> available{};
    available.set(2);
    available.set(5);
    available.set(8);
    available.set(11);

    PalettePool pool{available};

    // Check out index 8 specifically (skipping lower ones)
    pool.check_out(8);
    EXPECT_FALSE(pool.is_available(8));

    // Auto checkout should get lowest available (2)
    EXPECT_EQ(pool.check_out(), 2);

    // Check out 11 specifically
    pool.check_out(11);

    // Auto checkout should get 5 (only remaining)
    EXPECT_EQ(pool.check_out(), 5);

    EXPECT_FALSE(pool.has_available_index());
}

// =============================================================================
// check_in Tests
// =============================================================================

TEST(PalettePoolCheckInTests, ReturnsIndexToPool)
{
    std::bitset<pal::num_pals> available{};
    available.set(5);

    PalettePool pool{available};

    std::size_t index = pool.check_out();
    EXPECT_EQ(index, 5);
    EXPECT_FALSE(pool.has_available_index());

    pool.check_in();
    EXPECT_TRUE(pool.has_available_index());
}

TEST(PalettePoolCheckInTests, FollowsStackSemantics)
{
    std::bitset<pal::num_pals> available{};
    available.set(1);
    available.set(5);
    available.set(10);

    PalettePool pool{available};

    // Check out in order: 1, 5, 10
    EXPECT_EQ(pool.check_out(), 1);
    EXPECT_EQ(pool.check_out(), 5);
    EXPECT_EQ(pool.check_out(), 10);

    // Check in (LIFO): 10 is returned first
    pool.check_in();
    // Now 10 should be available again, check it out
    EXPECT_EQ(pool.check_out(), 10);

    // Check in 10, then 5
    pool.check_in();
    pool.check_in();
    // Now both 5 and 10 should be available, lowest (5) is returned
    EXPECT_EQ(pool.check_out(), 5);
}

TEST(PalettePoolCheckInTests, CanReuseCheckedInIndex)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);

    PalettePool pool{available};

    // Check out and in multiple times
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(pool.check_out(), 0);
        pool.check_in();
    }

    EXPECT_TRUE(pool.has_available_index());
}

TEST(PalettePoolCheckInTests, PanicsWhenStackEmpty)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);

    PalettePool pool{available};

    EXPECT_DEATH(pool.check_in(), "empty checkout stack");
}

TEST(PalettePoolCheckInTests, PanicsAfterAllCheckedIn)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);

    PalettePool pool{available};

    std::ignore = pool.check_out();
    pool.check_in();

    EXPECT_DEATH(pool.check_in(), "empty checkout stack");
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST(PalettePoolIntegrationTests, FullCheckoutAndCheckinCycle)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);
    available.set(3);
    available.set(7);
    available.set(15);

    PalettePool pool{available};

    // Check out all 4
    EXPECT_EQ(pool.check_out(), 0);
    EXPECT_EQ(pool.check_out(), 3);
    EXPECT_EQ(pool.check_out(), 7);
    EXPECT_EQ(pool.check_out(), 15);
    EXPECT_FALSE(pool.has_available_index());

    // Check in all 4 (LIFO order)
    pool.check_in(); // returns 15
    pool.check_in(); // returns 7
    pool.check_in(); // returns 3
    pool.check_in(); // returns 0

    // All should be available again
    EXPECT_TRUE(pool.has_available_index());

    // Check out should return lowest again
    EXPECT_EQ(pool.check_out(), 0);
}

TEST(PalettePoolIntegrationTests, PartialCheckoutAndCheckin)
{
    std::bitset<pal::num_pals> available{};
    available.set(2);
    available.set(4);
    available.set(6);
    available.set(8);

    PalettePool pool{available};

    // Check out 2 and 4
    EXPECT_EQ(pool.check_out(), 2);
    EXPECT_EQ(pool.check_out(), 4);

    // Check in 4
    pool.check_in();

    // Next checkout should get 4 (lowest available)
    EXPECT_EQ(pool.check_out(), 4);

    // Check out 6
    EXPECT_EQ(pool.check_out(), 6);

    // Check in 6, then 4
    pool.check_in();
    pool.check_in();

    // Lowest available should be 4
    EXPECT_EQ(pool.check_out(), 4);
}

TEST(PalettePoolIntegrationTests, SingleIndexPool)
{
    std::bitset<pal::num_pals> available{};
    available.set(7);

    PalettePool pool{available};

    EXPECT_TRUE(pool.has_available_index());
    EXPECT_EQ(pool.check_out(), 7);
    EXPECT_FALSE(pool.has_available_index());
    pool.check_in();
    EXPECT_TRUE(pool.has_available_index());
    EXPECT_EQ(pool.check_out(), 7);
}

TEST(PalettePoolIntegrationTests, AllSixteenIndexesAvailable)
{
    std::bitset<pal::num_pals> available{};
    available.set(); // All 16 bits set

    PalettePool pool{available};

    // Check out all 16
    for (std::size_t i = 0; i < pal::num_pals; ++i) {
        EXPECT_TRUE(pool.has_available_index());
        EXPECT_EQ(pool.check_out(), i);
    }

    EXPECT_FALSE(pool.has_available_index());

    // Check in all 16
    for (std::size_t i = 0; i < pal::num_pals; ++i) {
        pool.check_in();
    }

    EXPECT_TRUE(pool.has_available_index());
    EXPECT_EQ(pool.check_out(), 0);
}
