#include "gtest/gtest.h"

#include <string>
#include <utility>
#include <vector>

#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/infra/services/color_palette_printer.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"

using namespace porytiles;

class ColorPalettePrinterTests : public ::testing::Test {
  protected:
    PlainTextFormatter format_{};
    ColorPalettePrinter printer_{&format_};
};

TEST_F(ColorPalettePrinterTests, RgbaCounts)
{
    const std::vector<std::pair<Rgba32, unsigned int>> counts{{rgba_white, 3}, {Rgba32{234, 21, 97}, 1}};

    const auto lines = printer_.print_rgba_counts(counts);

    ASSERT_EQ(lines.size(), 2);
    EXPECT_EQ(lines.at(0), "255 255 255 ➞ 3 pixel(s)");
    EXPECT_EQ(lines.at(1), "234 21 97 ➞ 1 pixel(s)");
}

TEST_F(ColorPalettePrinterTests, RgbaCountsWithShare)
{
    const std::vector<std::pair<Rgba32, unsigned int>> counts{{rgba_white, 3}, {Rgba32{234, 21, 97}, 1}};

    const auto lines = printer_.print_rgba_counts(counts, 8);

    ASSERT_EQ(lines.size(), 2);
    EXPECT_EQ(lines.at(0), "255 255 255 ➞ 3 pixel(s) (37.50%)");
    EXPECT_EQ(lines.at(1), "234 21 97 ➞ 1 pixel(s) (12.50%)");
}
