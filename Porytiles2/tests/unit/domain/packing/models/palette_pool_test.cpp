#include "porytiles2/domain/packing/models/palette_pool.hpp"

#include <bitset>

#include "gtest/gtest.h"

using namespace porytiles2;

TEST(PalettePoolConstructorTests, ConstructAvailable)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);
    available.set(5);
    available.set(10);

    PalettePool pool{available};

    EXPECT_TRUE(pool.has_available_pal());
}

TEST(PalettePoolConstructorTests, ConstructNoneAvailable)
{
    std::bitset<pal::num_pals> available{};

    PalettePool pool{available};

    EXPECT_FALSE(pool.has_available_pal());
}

TEST(PalettePoolConstructorTests, ConstructAllAvailable)
{
    std::bitset<pal::num_pals> available{};
    available.set(); // Set all bits

    PalettePool pool{available};

    EXPECT_TRUE(pool.has_available_pal());
}

TEST(PalettePoolHasAvailableTests, WhenAvailable)
{
    std::bitset<pal::num_pals> available{};
    available.set(3);

    PalettePool pool{available};

    EXPECT_TRUE(pool.has_available_pal());
}

TEST(PalettePoolHasAvailableTests, WhenAllCheckedOut)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);

    PalettePool pool{available};

    std::ignore = pool.checkout();

    EXPECT_FALSE(pool.has_available_pal());
}

TEST(PalettePoolHasAvailableTests, AfterCheckIn)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);

    PalettePool pool{available};

    std::ignore = pool.checkout();
    EXPECT_FALSE(pool.has_available_pal());

    pool.checkin();
    EXPECT_TRUE(pool.has_available_pal());
}

TEST(PalettePoolIsAvailableTests, AvailableIndex)
{
    std::bitset<pal::num_pals> available{};
    available.set(3);
    available.set(7);

    PalettePool pool{available};

    EXPECT_TRUE(pool.is_available(3));
    EXPECT_TRUE(pool.is_available(7));
}

TEST(PalettePoolIsAvailableTests, UnavailableIndex)
{
    std::bitset<pal::num_pals> available{};
    available.set(3);

    PalettePool pool{available};

    EXPECT_FALSE(pool.is_available(0));
    EXPECT_FALSE(pool.is_available(5));
    EXPECT_FALSE(pool.is_available(15));
}

TEST(PalettePoolIsAvailableTests, CheckedOutIndex)
{
    std::bitset<pal::num_pals> available{};
    available.set(3);
    available.set(7);

    PalettePool pool{available};

    std::ignore = pool.checkout(); // checks out 3
    EXPECT_FALSE(pool.is_available(3));
    EXPECT_TRUE(pool.is_available(7));
}

TEST(PalettePoolIsAvailableTests, AfterCheckIn)
{
    std::bitset<pal::num_pals> available{};
    available.set(5);

    PalettePool pool{available};

    std::ignore = pool.checkout();
    EXPECT_FALSE(pool.is_available(5));

    pool.checkin();
    EXPECT_TRUE(pool.is_available(5));
}

TEST(PalettePoolIsAvailableTests, PanicsOutOfBounds)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);

    PalettePool pool{available};

    EXPECT_DEATH(std::ignore = pool.is_available(16), "out of bounds");
    EXPECT_DEATH(std::ignore = pool.is_available(100), "out of bounds");
}

TEST(PalettePoolCheckOutTests, LowestAvailable)
{
    std::bitset<pal::num_pals> available{};
    available.set(5);
    available.set(10);
    available.set(15);

    PalettePool pool{available};

    EXPECT_EQ(pool.checkout(), 5);
}

TEST(PalettePoolCheckOutTests, NextLowestAfterCheckout)
{
    std::bitset<pal::num_pals> available{};
    available.set(2);
    available.set(7);
    available.set(12);

    PalettePool pool{available};

    EXPECT_EQ(pool.checkout(), 2);
    EXPECT_EQ(pool.checkout(), 7);
    EXPECT_EQ(pool.checkout(), 12);
}

TEST(PalettePoolCheckOutTests, AllAvailable)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);
    available.set(1);
    available.set(2);

    PalettePool pool{available};

    std::ignore = pool.checkout();
    std::ignore = pool.checkout();
    std::ignore = pool.checkout();

    EXPECT_FALSE(pool.has_available_pal());
}

TEST(PalettePoolCheckOutTests, SkipsUnavailable)
{
    std::bitset<pal::num_pals> available{};
    // Only indexes 3, 7, 11 are available (not 0, 1, 2, etc.)
    available.set(3);
    available.set(7);
    available.set(11);

    PalettePool pool{available};

    EXPECT_EQ(pool.checkout(), 3);
    EXPECT_EQ(pool.checkout(), 7);
    EXPECT_EQ(pool.checkout(), 11);
}

TEST(PalettePoolCheckOutTests, PanicsNoneAvailable)
{
    std::bitset<pal::num_pals> available{};

    PalettePool pool{available};

    EXPECT_DEATH(std::ignore = pool.checkout(), "no available indexes");
}

TEST(PalettePoolCheckOutTests, PanicsAllCheckedOut)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);

    PalettePool pool{available};

    std::ignore = pool.checkout();

    EXPECT_DEATH(std::ignore = pool.checkout(), "no available indexes");
}

TEST(PalettePoolCheckOutIndexTests, SpecificIndex)
{
    std::bitset<pal::num_pals> available{};
    available.set(3);
    available.set(7);
    available.set(10);

    PalettePool pool{available};

    // Check out index 7 (not the lowest)
    pool.checkout(7);
    EXPECT_FALSE(pool.is_available(7));
    EXPECT_TRUE(pool.is_available(3));
    EXPECT_TRUE(pool.is_available(10));
}

TEST(PalettePoolCheckOutIndexTests, StackOrdering)
{
    std::bitset<pal::num_pals> available{};
    available.set(3);
    available.set(7);

    PalettePool pool{available};

    // Check out specific indexes in non-sequential order
    pool.checkout(7);
    pool.checkout(3);

    // Check in follows LIFO - should return 3 first
    pool.checkin();
    EXPECT_TRUE(pool.is_available(3));
    EXPECT_FALSE(pool.is_available(7));

    pool.checkin();
    EXPECT_TRUE(pool.is_available(7));
}

TEST(PalettePoolCheckOutIndexTests, PanicsUnavailable)
{
    std::bitset<pal::num_pals> available{};
    available.set(3);

    PalettePool pool{available};

    EXPECT_DEATH(pool.checkout(5), "not available");
}

TEST(PalettePoolCheckOutIndexTests, PanicsAlreadyCheckedOut)
{
    std::bitset<pal::num_pals> available{};
    available.set(3);

    PalettePool pool{available};

    pool.checkout(3);

    EXPECT_DEATH(pool.checkout(3), "not available");
}

TEST(PalettePoolCheckOutIndexTests, PanicsOutOfBounds)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);

    PalettePool pool{available};

    EXPECT_DEATH(pool.checkout(16), "out of bounds");
    EXPECT_DEATH(pool.checkout(100), "out of bounds");
}

TEST(PalettePoolCheckOutIndexTests, MixedAutoAndIndexed)
{
    std::bitset<pal::num_pals> available{};
    available.set(2);
    available.set(5);
    available.set(8);
    available.set(11);

    PalettePool pool{available};

    // Check out index 8 specifically (skipping lower ones)
    pool.checkout(8);
    EXPECT_FALSE(pool.is_available(8));

    // Auto checkout should get lowest available (2)
    EXPECT_EQ(pool.checkout(), 2);

    // Check out 11 specifically
    pool.checkout(11);

    // Auto checkout should get 5 (only remaining)
    EXPECT_EQ(pool.checkout(), 5);

    EXPECT_FALSE(pool.has_available_pal());
}

TEST(PalettePoolCheckInTests, BasicCheckIn)
{
    std::bitset<pal::num_pals> available{};
    available.set(5);

    PalettePool pool{available};

    std::size_t index = pool.checkout();
    EXPECT_EQ(index, 5);
    EXPECT_FALSE(pool.has_available_pal());

    pool.checkin();
    EXPECT_TRUE(pool.has_available_pal());
}

TEST(PalettePoolCheckInTests, StackSemantics)
{
    std::bitset<pal::num_pals> available{};
    available.set(1);
    available.set(5);
    available.set(10);

    PalettePool pool{available};

    // Check out in order: 1, 5, 10
    EXPECT_EQ(pool.checkout(), 1);
    EXPECT_EQ(pool.checkout(), 5);
    EXPECT_EQ(pool.checkout(), 10);

    // Check in (LIFO): 10 is returned first
    pool.checkin();
    // Now 10 should be available again, check it out
    EXPECT_EQ(pool.checkout(), 10);

    // Check in 10, then 5
    pool.checkin();
    pool.checkin();
    // Now both 5 and 10 should be available, lowest (5) is returned
    EXPECT_EQ(pool.checkout(), 5);
}

TEST(PalettePoolCheckInTests, ReuseCheckedIn)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);

    PalettePool pool{available};

    // Check out and in multiple times
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(pool.checkout(), 0);
        pool.checkin();
    }

    EXPECT_TRUE(pool.has_available_pal());
}

TEST(PalettePoolCheckInTests, PanicsEmpty)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);

    PalettePool pool{available};

    EXPECT_DEATH(pool.checkin(), "empty checkout stack");
}

TEST(PalettePoolCheckInTests, PanicsAllCheckedIn)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);

    PalettePool pool{available};

    std::ignore = pool.checkout();
    pool.checkin();

    EXPECT_DEATH(pool.checkin(), "empty checkout stack");
}

TEST(PalettePoolIntegrationTests, FullCycle)
{
    std::bitset<pal::num_pals> available{};
    available.set(0);
    available.set(3);
    available.set(7);
    available.set(15);

    PalettePool pool{available};

    // Check out all 4
    EXPECT_EQ(pool.checkout(), 0);
    EXPECT_EQ(pool.checkout(), 3);
    EXPECT_EQ(pool.checkout(), 7);
    EXPECT_EQ(pool.checkout(), 15);
    EXPECT_FALSE(pool.has_available_pal());

    // Check in all 4 (LIFO order)
    pool.checkin(); // returns 15
    pool.checkin(); // returns 7
    pool.checkin(); // returns 3
    pool.checkin(); // returns 0

    // All should be available again
    EXPECT_TRUE(pool.has_available_pal());

    // Check out should return lowest again
    EXPECT_EQ(pool.checkout(), 0);
}

TEST(PalettePoolIntegrationTests, PartialCycle)
{
    std::bitset<pal::num_pals> available{};
    available.set(2);
    available.set(4);
    available.set(6);
    available.set(8);

    PalettePool pool{available};

    // Check out 2 and 4
    EXPECT_EQ(pool.checkout(), 2);
    EXPECT_EQ(pool.checkout(), 4);

    // Check in 4
    pool.checkin();

    // Next checkout should get 4 (lowest available)
    EXPECT_EQ(pool.checkout(), 4);

    // Check out 6
    EXPECT_EQ(pool.checkout(), 6);

    // Check in 6, then 4
    pool.checkin();
    pool.checkin();

    // Lowest available should be 4
    EXPECT_EQ(pool.checkout(), 4);
}

TEST(PalettePoolIntegrationTests, SingleIndexPool)
{
    std::bitset<pal::num_pals> available{};
    available.set(7);

    PalettePool pool{available};

    EXPECT_TRUE(pool.has_available_pal());
    EXPECT_EQ(pool.checkout(), 7);
    EXPECT_FALSE(pool.has_available_pal());
    pool.checkin();
    EXPECT_TRUE(pool.has_available_pal());
    EXPECT_EQ(pool.checkout(), 7);
}

TEST(PalettePoolIntegrationTests, AllSixteenIndexes)
{
    std::bitset<pal::num_pals> available{};
    available.set(); // All 16 bits set

    PalettePool pool{available};

    // Check out all 16
    for (std::size_t i = 0; i < pal::num_pals; ++i) {
        EXPECT_TRUE(pool.has_available_pal());
        EXPECT_EQ(pool.checkout(), i);
    }

    EXPECT_FALSE(pool.has_available_pal());

    // Check in all 16
    for (std::size_t i = 0; i < pal::num_pals; ++i) {
        pool.checkin();
    }

    EXPECT_TRUE(pool.has_available_pal());
    EXPECT_EQ(pool.checkout(), 0);
}
