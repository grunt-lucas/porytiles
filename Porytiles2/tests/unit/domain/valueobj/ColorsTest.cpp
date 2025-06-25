#include "gtest/gtest.h"

#include "fmt/format.h"

#include "porytiles2/domain/valueobj/Bgr15.hpp"
#include "porytiles2/domain/valueobj/Rgba32.hpp"

using namespace porytiles;

TEST(Bgr15Tests, ClassInvariantShouldHold) {
    constexpr Bgr15 bgr_red{253, 0, 0};
    EXPECT_EQ(bgr_red.red(), 248);

    constexpr Bgr15 bgr_green{0, 251, 0};
    EXPECT_EQ(bgr_green.green(), 248);

    constexpr Bgr15 bgr_blue{0, 0, 255};
    EXPECT_EQ(bgr_blue.blue(), 248);
}

TEST(Bgr15Tests, PackAndUnpackShouldWork) {
    constexpr Bgr15 bgr_red{253, 0, 0};
    const auto bgr_red_packed = bgr_red.Pack();
    const auto bgr_red_unpacked = Bgr15::Unpack(bgr_red_packed);
    EXPECT_EQ(bgr_red_packed, 31);
    EXPECT_EQ(bgr_red_unpacked, bgr_red);

    constexpr Bgr15 bgr_green{0, 251, 0};
    const auto bgr_green_packed = bgr_green.Pack();
    const auto bgr_green_unpacked = Bgr15::Unpack(bgr_green_packed);
    EXPECT_EQ(bgr_green.Pack(), 992);
    EXPECT_EQ(bgr_green_unpacked, bgr_green);

    constexpr Bgr15 bgr_blue{0, 0, 255};
    const auto bgr_blue_packed = bgr_blue.Pack();
    const auto bgr_blue_unpacked = Bgr15::Unpack(bgr_blue_packed);
    EXPECT_EQ(bgr_blue.Pack(), 31744);
    EXPECT_EQ(bgr_blue_unpacked, bgr_blue);
}

TEST(Bgr15Tests, FmtlibFormattingShouldUseJasc) {
    constexpr Bgr15 bgr1{127, 12, 222};
    const auto formatted = fmt::format("{}", bgr1);
    EXPECT_EQ(formatted, "120 8 216");
}

TEST(Rgba32Tests, ToJascStrShouldWork) {
    constexpr Rgba32 rgba1{127, 12, 222};
    EXPECT_EQ(rgba1.ToJascStr(), "127 12 222");
}

TEST(Rgba32Tests, EqualsIgnoringAlphaShouldWork) {
    constexpr Rgba32 rgba1{127, 12, 222, Rgba32::kAlphaOpaque};
    constexpr Rgba32 rgba2{127, 12, 222, Rgba32::kAlphaTransparent};
    EXPECT_TRUE(rgba1.EqualsIgnoringAlpha(rgba2));

    constexpr Rgba32 rgba3{127, 10, 222, Rgba32::kAlphaOpaque};
    constexpr Rgba32 rgba4{127, 12, 222, Rgba32::kAlphaTransparent};
    EXPECT_FALSE(rgba3.EqualsIgnoringAlpha(rgba4));
}

TEST(Rgba32Tests, FmtlibFormattingShouldUseJasc) {
    constexpr Rgba32 rgba1{127, 12, 222};
    const auto formatted = fmt::format("{}", rgba1);
    EXPECT_EQ(formatted, "127 12 222");
}
