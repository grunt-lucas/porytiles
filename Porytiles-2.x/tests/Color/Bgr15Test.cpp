#include <gtest/gtest.h>

#include <cmath>

#include "porytiles/Color/Bgr15.h"

// Just a fun little example of template metaprogramming
template <int A, int B> struct ComputeExponent {
    static constexpr int value = A * ComputeExponent<A, B - 1>::value;
};
template <int A> struct ComputeExponent<A, 0> {
    static constexpr int value = 1;
};

constexpr int TWO_FIFTEENTH_POW_MINUS_ONE = static_cast<int>(ComputeExponent<2, 15>::value) - 1;

TEST(Bgr15Test, TestDefaultCtor)
{
    using namespace porytiles::color;

    constexpr Bgr15 bgr{};
    EXPECT_EQ(bgr.getRawValue(), 0);
}

TEST(Bgr15Test, TestComponentCtor)
{
    using namespace porytiles::color;

    constexpr Bgr15 bgr{255, 255, 255};
    EXPECT_EQ(bgr.getRawValue(), TWO_FIFTEENTH_POW_MINUS_ONE);
}
