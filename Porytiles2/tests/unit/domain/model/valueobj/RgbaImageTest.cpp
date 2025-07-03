#include <gtest/gtest.h>

#include <tuple>

#include <fmt/format.h>

#include <porytiles2/domain/model/valueobj/Rgba32.hpp>
#include <porytiles2/domain/model/valueobj/RgbaTile.hpp>

using namespace porytiles;

// TEST(RgbaImageImplTest, DimensionsMethodsShouldWork) {
//   cimg_library::CImg<std::uint8_t> image{};
//   image.assign("Resources/Tests/Unit/png/pattern.png");
//   const std::unique_ptr<RgbaImage> png = std::make_unique<RgbaImagePng>(image);
//
//   EXPECT_EQ(png->Width(), 16);
//   EXPECT_EQ(png->Height(), 16);
// }
//
// TEST(RgbaImageImplTest, GetByRowColShouldWork) {
//   cimg_library::CImg<std::uint8_t> image{};
//   image.assign("Resources/Tests/Unit/png/pattern.png");
//   const std::unique_ptr<RgbaImage> png = std::make_unique<RgbaImagePng>(image);
//
//   constexpr Rgba32 red{255, 0, 0};
//   constexpr Rgba32 green{0, 255, 0};
//   constexpr Rgba32 blue{0, 0, 255};
//   constexpr Rgba32 magenta{255, 0, 255};
//   constexpr Rgba32 cyan{0, 255, 255};
//
//   EXPECT_EQ(png->At(0, 0), blue);
//   EXPECT_EQ(png->At(7, 15), red);
//   EXPECT_EQ(png->At(7, 14), green);
//   EXPECT_EQ(png->At(0, 8), magenta);
//   EXPECT_EQ(png->At(8, 0), cyan);
// }