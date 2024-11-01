#include <gtest/gtest.h>

#include "porytiles/Color/RGBLike.h"
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

    const Bgr15 bgr{255, 255, 255};
    EXPECT_EQ(bgr.getRawValue(), TWO_FIFTEENTH_POW_MINUS_ONE);
}

TEST(Bgr15Test, TestComponentGetters)
{
    using namespace porytiles::color;

    const Bgr15 bgr{255, 255, 255};
    // expected component value is 248 due to precision loss
    constexpr std::uint8_t expectedComponentValue = 248;
    EXPECT_EQ(bgr.computeRedComponent(), expectedComponentValue);
    EXPECT_EQ(bgr.computeGreenComponent(), expectedComponentValue);
    EXPECT_EQ(bgr.computeBlueComponent(), expectedComponentValue);
    EXPECT_EQ(bgr.computeAlphaComponent(), ALPHA_OPAQUE);
}
