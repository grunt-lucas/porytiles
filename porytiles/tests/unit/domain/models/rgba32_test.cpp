#include "gtest/gtest.h"

#include <format>

#include "porytiles/domain/models/rgba32.hpp"

static_assert(std::formattable<porytiles::Rgba32, char>, "Rgba32 must satisfy std::formattable");

using namespace porytiles;

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
    // Formatter uses porytiles::to_string() for consistent formatting across codebase
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

TEST(Rgba32Tests, GbaChannelConversion)
{
    EXPECT_EQ(gba_downconvert_channel(0), 0);
    EXPECT_EQ(gba_downconvert_channel(7), 0);
    EXPECT_EQ(gba_downconvert_channel(8), 1);
    EXPECT_EQ(gba_downconvert_channel(255), 31);
    EXPECT_EQ(gba_upconvert_channel(0), 0);
    EXPECT_EQ(gba_upconvert_channel(1), 8);
    EXPECT_EQ(gba_upconvert_channel(12), 98);
    EXPECT_EQ(gba_upconvert_channel(31), 255);
}

TEST(Rgba32Tests, QuantizeToGba)
{
    EXPECT_EQ(rgba_white.quantize_to_gba(), rgba_white);
    EXPECT_EQ(rgba_black.quantize_to_gba(), rgba_black);
    // 250 -> 31 -> 255, 16 -> 2 -> 16, 100 -> 12 -> 98; alpha passes through.
    EXPECT_EQ((Rgba32{250, 16, 100, 7}).quantize_to_gba(), (Rgba32{255, 16, 98, 7}));
    // Quantizing is idempotent.
    const Rgba32 once = Rgba32{123, 45, 67}.quantize_to_gba();
    EXPECT_EQ(once.quantize_to_gba(), once);
}

TEST(Rgba32Tests, ParseRgba32StringThreeComponents)
{
    const auto parsed = parse_rgba32_string("234,21,97");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed.value(), (Rgba32{234, 21, 97, Rgba32::alpha_opaque}));
}

TEST(Rgba32Tests, ParseRgba32StringFourComponentsAndWhitespace)
{
    const auto parsed = parse_rgba32_string(" 1, 2 ,3 , 0 ");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed.value(), (Rgba32{1, 2, 3, Rgba32::alpha_transparent}));
}

TEST(Rgba32Tests, ParseRgba32StringRejectsBadInput)
{
    EXPECT_EQ(parse_rgba32_string("1,2").error(), "expected R,G,B or R,G,B,A format (got 2 components)");
    EXPECT_EQ(parse_rgba32_string("1,2,3,4,5").error(), "expected R,G,B or R,G,B,A format (got 5 components)");
    EXPECT_EQ(parse_rgba32_string("1,x,3").error(), "'x' is not a valid integer");
    EXPECT_EQ(parse_rgba32_string("1,,3").error(), "'' is not a valid integer");
    EXPECT_EQ(parse_rgba32_string("1,256,3").error(), "component 256 is out of range (must be 0-255)");
    EXPECT_EQ(parse_rgba32_string("-1,0,0").error(), "component -1 is out of range (must be 0-255)");
    EXPECT_FALSE(parse_rgba32_string("").has_value());
}
