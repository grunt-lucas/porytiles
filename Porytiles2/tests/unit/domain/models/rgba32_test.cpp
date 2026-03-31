#include "gtest/gtest.h"

#include <format>

#include "porytiles2/domain/models/rgba32.hpp"

static_assert(std::formattable<porytiles2::Rgba32, char>, "Rgba32 must satisfy std::formattable");

using namespace porytiles2;

TEST(Rgba32Tests, ToJascStr)
{
    constexpr Rgba32 rgba1{127, 12, 222};
    EXPECT_EQ(rgba1.to_jasc_str(), "127 12 222");
}

TEST(Rgba32Tests, EqualsIgnoringAlpha)
{
    constexpr Rgba32 rgba1{127, 12, 222, Rgba32::alpha_opaque};
    constexpr Rgba32 rgba2{127, 12, 222, Rgba32::alpha_transparent};
    EXPECT_TRUE(rgba1.equals_ignoring_alpha(rgba2));

    constexpr Rgba32 rgba3{127, 10, 222, Rgba32::alpha_opaque};
    constexpr Rgba32 rgba4{127, 12, 222, Rgba32::alpha_transparent};
    EXPECT_FALSE(rgba3.equals_ignoring_alpha(rgba4));
}

TEST(Rgba32Tests, StdFormatUsesToString)
{
    constexpr Rgba32 rgba1{127, 12, 222};
    const auto formatted = std::format("{}", rgba1);
    // Formatter uses porytiles2::to_string() for consistent formatting across codebase
    EXPECT_EQ(formatted, "[127, 12, 222, 255]");
}

TEST(Rgba32Tests, EqualityConsidersAlpha)
{
    constexpr Rgba32 rgba1{127, 12, 222, Rgba32::alpha_opaque};
    constexpr Rgba32 rgba2{127, 12, 222, Rgba32::alpha_transparent};
    EXPECT_FALSE(rgba1 == rgba2);

    constexpr Rgba32 rgba3{127, 12, 222, Rgba32::alpha_opaque};
    constexpr Rgba32 rgba4{127, 12, 222, Rgba32::alpha_opaque};
    EXPECT_TRUE(rgba3 == rgba4);
}

TEST(Rgba32Tests, EqualityVsIgnoringAlpha)
{
    constexpr Rgba32 rgba1{100, 50, 200, Rgba32::alpha_opaque};
    constexpr Rgba32 rgba2{100, 50, 200, Rgba32::alpha_transparent};

    // operator== should consider alpha, so these should be different
    EXPECT_FALSE(rgba1 == rgba2);

    // equals_ignoring_alpha should ignore alpha, so these should be equal
    EXPECT_TRUE(rgba1.equals_ignoring_alpha(rgba2));
}

TEST(Rgba32Tests, DefaultIsTransparent)
{
    const Rgba32 default_rgba{};
    const Rgba32 transparent_ref{0, 0, 0, Rgba32::alpha_transparent};
    EXPECT_TRUE(default_rgba.is_transparent(transparent_ref));
    EXPECT_EQ(default_rgba.alpha(), Rgba32::alpha_transparent);
}
