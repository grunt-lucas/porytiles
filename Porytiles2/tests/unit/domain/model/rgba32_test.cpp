#include "gtest/gtest.h"

#include "fmt/format.h"

#include "porytiles2/domain/model/rgba32.hpp"

using namespace porytiles2;

TEST(Rgba32Tests, ToJascStrShouldWork)
{
    constexpr Rgba32 rgba1{127, 12, 222};
    EXPECT_EQ(rgba1.to_jasc_str(), "127 12 222");
}

TEST(Rgba32Tests, EqualsIgnoringAlphaShouldWork)
{
    constexpr Rgba32 rgba1{127, 12, 222, Rgba32::alpha_opaque};
    constexpr Rgba32 rgba2{127, 12, 222, Rgba32::alpha_transparent};
    EXPECT_TRUE(rgba1.equals_ignoring_alpha(rgba2));

    constexpr Rgba32 rgba3{127, 10, 222, Rgba32::alpha_opaque};
    constexpr Rgba32 rgba4{127, 12, 222, Rgba32::alpha_transparent};
    EXPECT_FALSE(rgba3.equals_ignoring_alpha(rgba4));
}

TEST(Rgba32Tests, FmtlibFormattingShouldUseJasc)
{
    constexpr Rgba32 rgba1{127, 12, 222};
    const auto formatted = fmt::format("{}", rgba1);
    EXPECT_EQ(formatted, "127 12 222");
}
