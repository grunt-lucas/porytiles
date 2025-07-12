#include <gtest/gtest.h>

#include <porytiles2/domain/model/valueobj/rgba_image.hpp>

using namespace porytiles2;

TEST(RgbaImageTest, DimensionsMethodsShouldWork) {
  constexpr std::size_t width = 10;
  constexpr std::size_t height = 20;
  RgbaImage image{width, height};

  EXPECT_EQ(image.width(), width);
  EXPECT_EQ(image.height(), height);
}

TEST(RgbaImageTest, AtByIndexShouldWork) {
  constexpr std::size_t width = 3;
  constexpr std::size_t height = 2;
  RgbaImage image{width, height};

  constexpr Rgba32 red_pixel{255, 0, 0, 255};
  constexpr Rgba32 green_pixel{0, 255, 0, 255};
  constexpr Rgba32 blue_pixel{0, 0, 255, 255};

  image.set(0, red_pixel);
  image.set(1, green_pixel);
  image.set(5, blue_pixel);

  EXPECT_EQ(image.at(0), red_pixel);
  EXPECT_EQ(image.at(1), green_pixel);
  EXPECT_EQ(image.at(5), blue_pixel);
}

TEST(RgbaImageTest, AtByRowColShouldWork) {
  constexpr std::size_t width = 3;
  constexpr std::size_t height = 2;
  RgbaImage image{width, height};

  constexpr Rgba32 red_pixel{255, 0, 0, 255};
  constexpr Rgba32 green_pixel{0, 255, 0, 255};
  constexpr Rgba32 blue_pixel{0, 0, 255, 255};

  image.set(0, 0, red_pixel);
  image.set(0, 1, green_pixel);
  image.set(1, 2, blue_pixel);

  EXPECT_EQ(image.at(0, 0), red_pixel);
  EXPECT_EQ(image.at(0, 1), green_pixel);
  EXPECT_EQ(image.at(1, 2), blue_pixel);
}

TEST(RgbaImageTest, SetByIndexShouldWork) {
  constexpr std::size_t width = 4;
  constexpr std::size_t height = 3;
  RgbaImage image{width, height};

  constexpr Rgba32 yellow_pixel{255, 255, 0, 128};
  constexpr Rgba32 magenta_pixel{255, 0, 255, 200};

  image.set(0, yellow_pixel);
  image.set(11, magenta_pixel);

  EXPECT_EQ(image.at(0), yellow_pixel);
  EXPECT_EQ(image.at(11), magenta_pixel);
}

TEST(RgbaImageTest, SetByRowColShouldWork) {
  constexpr std::size_t width = 4;
  constexpr std::size_t height = 3;
  RgbaImage image{width, height};

  constexpr Rgba32 cyan_pixel{0, 255, 255, 100};
  constexpr Rgba32 purple_pixel{128, 0, 255, 50};

  image.set(0, 0, cyan_pixel);
  image.set(2, 3, purple_pixel);

  EXPECT_EQ(image.at(0, 0), cyan_pixel);
  EXPECT_EQ(image.at(2, 3), purple_pixel);
}

TEST(RgbaImageTest, IndexAndRowColCorrespondence) {
  constexpr std::size_t width = 3;
  constexpr std::size_t height = 2;
  RgbaImage image{width, height};

  constexpr Rgba32 test_pixel{128, 64, 32, 16};

  // Set using row, col and verify using index
  image.set(1, 2, test_pixel);
  EXPECT_EQ(image.at(1 * width + 2), test_pixel);

  // Set using index and verify using row, col
  constexpr Rgba32 another_pixel{200, 100, 50, 25};
  image.set(0 * width + 1, another_pixel);
  EXPECT_EQ(image.at(0, 1), another_pixel);
}

TEST(RgbaImageTest, DefaultPixelValues) {
  constexpr std::size_t width = 2;
  constexpr std::size_t height = 2;
  RgbaImage image{width, height};

  // Default constructed Rgba32 should be {0, 0, 0, 0}
  constexpr Rgba32 default_pixel{};
  EXPECT_EQ(image.at(0, 0), default_pixel);
  EXPECT_EQ(image.at(0, 1), default_pixel);
  EXPECT_EQ(image.at(1, 0), default_pixel);
  EXPECT_EQ(image.at(1, 1), default_pixel);
}

TEST(RgbaImageTest, SinglePixelImage) {
  constexpr std::size_t width = 1;
  constexpr std::size_t height = 1;
  RgbaImage image{width, height};

  EXPECT_EQ(image.width(), 1);
  EXPECT_EQ(image.height(), 1);

  constexpr Rgba32 single_pixel{42, 84, 126, 168};
  image.set(0, 0, single_pixel);
  EXPECT_EQ(image.at(0, 0), single_pixel);
  EXPECT_EQ(image.at(0), single_pixel);
}

TEST(RgbaImageTest, AtByIndexOutOfBoundsPanic) {
  constexpr std::size_t width = 3;
  constexpr std::size_t height = 2;
  const RgbaImage image{width, height};

  EXPECT_EXIT(std::ignore = image.at(6), ::testing::KilledBySignal(SIGABRT),
              "index 6 out of bounds for image size 6");
  EXPECT_EXIT(std::ignore = image.at(100), ::testing::KilledBySignal(SIGABRT),
              "index 100 out of bounds for image size 6");
}

TEST(RgbaImageTest, AtByRowColColumnOutOfBoundsPanic) {
  constexpr std::size_t width = 3;
  constexpr std::size_t height = 2;
  const RgbaImage image{width, height};

  EXPECT_EXIT(std::ignore = image.at(0, 3), ::testing::KilledBySignal(SIGABRT),
              "col 3 out of bounds for image width 3");
  EXPECT_EXIT(std::ignore = image.at(1, 10), ::testing::KilledBySignal(SIGABRT),
              "col 10 out of bounds for image width 3");
}

TEST(RgbaImageTest, AtByRowColRowOutOfBoundsPanic) {
  constexpr std::size_t width = 3;
  constexpr std::size_t height = 2;
  const RgbaImage image{width, height};

  EXPECT_EXIT(std::ignore = image.at(2, 0), ::testing::KilledBySignal(SIGABRT),
              "row 2 out of bounds for image height 2");
  EXPECT_EXIT(std::ignore = image.at(5, 1), ::testing::KilledBySignal(SIGABRT),
              "row 5 out of bounds for image height 2");
}

TEST(RgbaImageTest, SetByIndexOutOfBoundsPanic) {
  constexpr std::size_t width = 2;
  constexpr std::size_t height = 2;
  RgbaImage image{width, height};
  constexpr Rgba32 test_pixel{255, 0, 0, 255};

  EXPECT_EXIT(image.set(4, test_pixel), ::testing::KilledBySignal(SIGABRT),
              "index 4 out of bounds for image size 4");
  EXPECT_EXIT(image.set(50, test_pixel), ::testing::KilledBySignal(SIGABRT),
              "index 50 out of bounds for image size 4");
}

TEST(RgbaImageTest, SetByRowColColumnOutOfBoundsPanic) {
  constexpr std::size_t width = 2;
  constexpr std::size_t height = 2;
  RgbaImage image{width, height};
  constexpr Rgba32 test_pixel{255, 0, 0, 255};

  EXPECT_EXIT(image.set(0, 2, test_pixel), ::testing::KilledBySignal(SIGABRT),
              "col 2 out of bounds for image width 2");
  EXPECT_EXIT(image.set(1, 5, test_pixel), ::testing::KilledBySignal(SIGABRT),
              "col 5 out of bounds for image width 2");
}

TEST(RgbaImageTest, SetByRowColRowOutOfBoundsPanic) {
  constexpr std::size_t width = 2;
  constexpr std::size_t height = 2;
  RgbaImage image{width, height};
  constexpr Rgba32 test_pixel{255, 0, 0, 255};

  EXPECT_EXIT(image.set(2, 0, test_pixel), ::testing::KilledBySignal(SIGABRT),
              "row 2 out of bounds for image height 2");
  EXPECT_EXIT(image.set(10, 1, test_pixel), ::testing::KilledBySignal(SIGABRT),
              "row 10 out of bounds for image height 2");
}