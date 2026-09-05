#include "gtest/gtest.h"

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/infra/services/ascii_tile_printer.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"

using namespace porytiles;

namespace {

// With the plain formatter every pixel cell is two characters: "XX" for a marker, "  " otherwise. Each rendered row is
// 16 cells plus the one-space gap between the west and east subtiles.
constexpr std::size_t row_width = metatile::side_length_pix * 2 + 1;

Metatile<Rgba32> solid_metatile(const Rgba32 &color)
{
    Metatile<Rgba32> metatile{};
    for (std::size_t i = 0; i < metatile::tiles_per_metatile_layer; ++i) {
        metatile.set_bottom(i, PixelTile<Rgba32>{color});
        metatile.set_middle(i, PixelTile<Rgba32>{color});
        metatile.set_top(i, PixelTile<Rgba32>{color});
    }
    return metatile;
}

std::size_t marker_column(std::size_t col)
{
    // Cells left of the gap sit at 2 * col; cells right of it shift by the gap character.
    return col < tile::side_length_pix ? 2 * col : 2 * col + 1;
}

std::size_t count_markers(const std::vector<std::string> &lines)
{
    std::size_t count = 0;
    for (const auto &line : lines) {
        for (std::size_t pos = line.find("XX"); pos != std::string::npos; pos = line.find("XX", pos + 2)) {
            count++;
        }
    }
    return count;
}

} // namespace

class AsciiTilePrinterTests : public ::testing::Test {
  protected:
    PlainTextFormatter format_{};
    AsciiTilePrinter printer_{&format_};
    Metatile<Rgba32> metatile_{solid_metatile(rgba_white)};
};

TEST_F(AsciiTilePrinterTests, MetatileLayerMarkersLandOnMetatileLocalCoords)
{
    const std::set<std::pair<std::size_t, std::size_t>> coords{{0, 0}, {3, 9}, {12, 15}};

    const auto lines =
        printer_.print_metatile_pixel_highlights(metatile_, metatile::Layer::bottom, coords, rgba_magenta);

    // Blank line, 8 rows, spacer, 8 rows, blank line.
    ASSERT_EQ(lines.size(), 19);
    EXPECT_TRUE(lines.front().empty());
    EXPECT_TRUE(lines.at(9).empty());
    EXPECT_TRUE(lines.back().empty());
    EXPECT_EQ(count_markers(lines), 3);

    // Row r is line r + 1 in the north half and r + 2 in the south half (after the spacer).
    EXPECT_EQ(lines.at(1).size(), row_width);
    EXPECT_EQ(lines.at(1).substr(marker_column(0), 2), "XX");
    EXPECT_EQ(lines.at(4).substr(marker_column(9), 2), "XX");
    EXPECT_EQ(lines.at(14).substr(marker_column(15), 2), "XX");
}

TEST_F(AsciiTilePrinterTests, MetatileLayerWithoutMarkersHasNoMarkers)
{
    const auto lines = printer_.print_metatile_pixel_highlights(metatile_, metatile::Layer::top, {}, rgba_magenta);

    ASSERT_EQ(lines.size(), 19);
    EXPECT_EQ(count_markers(lines), 0);
}

TEST_F(AsciiTilePrinterTests, MetatileLayerSubstitutesExtrinsicTransparencyForAlphaZero)
{
    // An alpha-0 pixel takes the extrinsic transparency color for rendering and is never a marker candidate: the
    // command's matcher already skips it, and the renderer must not choke on it either.
    PixelTile<Rgba32> tile{rgba_white};
    tile.set(0, 0, Rgba32{});
    metatile_.set_bottom(0, tile);

    const auto lines =
        printer_.print_metatile_pixel_highlights(metatile_, metatile::Layer::bottom, {{0, 1}}, rgba_magenta);

    ASSERT_EQ(lines.size(), 19);
    EXPECT_EQ(count_markers(lines), 1);
    EXPECT_EQ(lines.at(1).substr(marker_column(1), 2), "XX");
}

TEST_F(AsciiTilePrinterTests, SubtileMarkersMapToMetatilePosition)
{
    // Subtile-local (2, 3) in the southeast subtile is metatile-local (10, 11). The subtile overload also draws an
    // arrow line under the south subtiles.
    const auto lines = printer_.print_metatile_pixel_highlight(
        metatile_, metatile::Layer::middle, metatile::Subtile::southeast, 2, 3, rgba_magenta);

    ASSERT_EQ(lines.size(), 20);
    EXPECT_EQ(count_markers(lines), 1);
    EXPECT_EQ(lines.at(12).substr(marker_column(11), 2), "XX");
}

TEST_F(AsciiTilePrinterTests, SubtileIndexMarkersMapToMetatilePosition)
{
    // Row-major index 9 is subtile-local (1, 1); in the northeast subtile that is metatile-local (1, 9). The arrow
    // line for a north subtile comes before the rows.
    const auto lines = printer_.print_metatile_pixel_highlights(
        metatile_, metatile::Layer::bottom, metatile::Subtile::northeast, {9}, rgba_magenta);

    ASSERT_EQ(lines.size(), 20);
    EXPECT_EQ(count_markers(lines), 1);
    EXPECT_EQ(lines.at(3).substr(marker_column(9), 2), "XX");
}

TEST_F(AsciiTilePrinterTests, TileMarkers)
{
    PixelTile<Rgba32> tile{rgba_white};

    const auto lines = printer_.print_tile_pixel_highlights(tile, {0, 63}, rgba_magenta);

    // Blank line, 8 rows, blank line.
    ASSERT_EQ(lines.size(), 10);
    EXPECT_EQ(count_markers(lines), 2);
    EXPECT_EQ(lines.at(1).substr(0, 2), "XX");
    EXPECT_EQ(lines.at(8).substr(14, 2), "XX");
}
