#include <gtest/gtest.h>

#include <porytiles2/domain/model/valueobj/image.hpp>
#include <porytiles2/domain/model/valueobj/rgba32.hpp>

using namespace porytiles2;

using RgbaImage = Image<Rgba32>;

TEST(ImageTest, DimensionsMethodsShouldWork) {
    constexpr std::size_t width = 10;
    constexpr std::size_t height = 20;
    RgbaImage image{width, height};

    EXPECT_EQ(image.width(), width);
    EXPECT_EQ(image.height(), height);
}

TEST(ImageTest, AtByIndexShouldWork) {
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

TEST(ImageTest, AtByRowColShouldWork) {
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

TEST(ImageTest, SetByIndexShouldWork) {
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

TEST(ImageTest, SetByRowColShouldWork) {
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

TEST(ImageTest, IndexAndRowColCorrespondence) {
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

TEST(ImageTest, DefaultPixelValues) {
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

TEST(ImageTest, SinglePixelImage) {
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

TEST(ImageTest, AtByIndexOutOfBoundsPanic) {
    constexpr std::size_t width = 3;
    constexpr std::size_t height = 2;
    const RgbaImage image{width, height};

    EXPECT_DEATH(std::ignore = image.at(6), "index 6 out of bounds for image size 6");
    EXPECT_DEATH(std::ignore = image.at(100), "index 100 out of bounds for image size 6");
}

TEST(ImageTest, AtByRowColColumnOutOfBoundsPanic) {
    constexpr std::size_t width = 3;
    constexpr std::size_t height = 2;
    const RgbaImage image{width, height};

    EXPECT_DEATH(std::ignore = image.at(0, 3), "col 3 out of bounds for image width 3");
    EXPECT_DEATH(std::ignore = image.at(1, 10), "col 10 out of bounds for image width 3");
}

TEST(ImageTest, AtByRowColRowOutOfBoundsPanic) {
    constexpr std::size_t width = 3;
    constexpr std::size_t height = 2;
    const RgbaImage image{width, height};

    EXPECT_DEATH(std::ignore = image.at(2, 0), "row 2 out of bounds for image height 2");
    EXPECT_DEATH(std::ignore = image.at(5, 1), "row 5 out of bounds for image height 2");
}

TEST(ImageTest, SetByIndexOutOfBoundsPanic) {
    constexpr std::size_t width = 2;
    constexpr std::size_t height = 2;
    RgbaImage image{width, height};
    constexpr Rgba32 test_pixel{255, 0, 0, 255};

    EXPECT_DEATH(image.set(4, test_pixel), "index 4 out of bounds for image size 4");
    EXPECT_DEATH(image.set(50, test_pixel), "index 50 out of bounds for image size 4");
}

TEST(ImageTest, SetByRowColColumnOutOfBoundsPanic) {
    constexpr std::size_t width = 2;
    constexpr std::size_t height = 2;
    RgbaImage image{width, height};
    constexpr Rgba32 test_pixel{255, 0, 0, 255};

    EXPECT_DEATH(image.set(0, 2, test_pixel), "col 2 out of bounds for image width 2");
    EXPECT_DEATH(image.set(1, 5, test_pixel), "col 5 out of bounds for image width 2");
}

TEST(ImageTest, SetByRowColRowOutOfBoundsPanic) {
    constexpr std::size_t width = 2;
    constexpr std::size_t height = 2;
    RgbaImage image{width, height};
    constexpr Rgba32 test_pixel{255, 0, 0, 255};

    EXPECT_DEATH(image.set(2, 0, test_pixel), "row 2 out of bounds for image height 2");
    EXPECT_DEATH(image.set(10, 1, test_pixel), "row 10 out of bounds for image height 2");
}