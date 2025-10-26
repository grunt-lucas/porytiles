#include "gtest/gtest.h"

#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/services/tile_validator.hpp"
#include "porytiles2/infra/services/stderr_ascii_tile_printer.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

using namespace porytiles2;

TEST(TileValidatorTests, ValidateAlphaChannels_AllValidAlphaValues_ReturnsSuccess)
{
    // Create tiles with only valid alpha values (0 or 255)
    PixelTile<Rgba32> tile1{};
    for (std::size_t i = 0; i < tile::size_pix; i++) {
        // Set half the pixels to opaque, half to transparent
        if (i < tile::size_pix / 2) {
            tile1.set(i, Rgba32{100, 150, 200, Rgba32::alpha_opaque});
        }
        else {
            tile1.set(i, Rgba32{50, 75, 100, Rgba32::alpha_transparent});
        }
    }

    PixelTile<Rgba32> tile2{};
    for (std::size_t i = 0; i < tile::size_pix; i++) {
        // All pixels opaque
        tile2.set(i, Rgba32{200, 100, 50, Rgba32::alpha_opaque});
    }

    std::vector tiles = {tile1, tile2};

    PlainTextFormatter formatter{};
    BufferedUserDiagnostics diag{};
    StderrAsciiTilePrinter tile_printer{&formatter};
    TileValidator validator{&formatter, &diag, &tile_printer};

    auto result = validator.validate_alpha_channels(tiles);

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(diag.errors().empty());
}

TEST(TileValidatorTests, ValidateAlphaChannels_SomeInvalidAlphaValues_ReturnsFailure)
{
    // Create tiles with some invalid alpha values
    PixelTile<Rgba32> tile1{};
    for (std::size_t i = 0; i < tile::size_pix; i++) {
        if (i == 0) {
            // First pixel has invalid alpha value
            tile1.set(i, Rgba32{100, 150, 200, 128});
        }
        else {
            // Rest are valid
            tile1.set(i, Rgba32{100, 150, 200, Rgba32::alpha_opaque});
        }
    }

    PixelTile<Rgba32> tile2{};
    for (std::size_t i = 0; i < tile::size_pix; i++) {
        if (i == tile::size_pix - 1) {
            // Last pixel has invalid alpha value
            tile2.set(i, Rgba32{200, 100, 50, 64});
        }
        else {
            // Rest are valid
            tile2.set(i, Rgba32{200, 100, 50, Rgba32::alpha_transparent});
        }
    }

    std::vector<PixelTile<Rgba32>> tiles = {tile1, tile2};

    PlainTextFormatter formatter{};
    BufferedUserDiagnostics diag{};
    StderrAsciiTilePrinter tile_printer{&formatter};
    TileValidator validator{&formatter, &diag, &tile_printer};

    auto result = validator.validate_alpha_channels(tiles);

    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(diag.errors().empty());

    // Verify that error messages were generated
    EXPECT_EQ(diag.errors().size(), 2);

    // Check that the error messages contain information about invalid alpha values
    const auto &first_error = diag.errors()[0];
    EXPECT_FALSE(first_error.empty());
    bool found_alpha_128 = false;
    for (const auto &line : first_error) {
        if (line.find("128") != std::string::npos) {
            found_alpha_128 = true;
            break;
        }
    }
    EXPECT_TRUE(found_alpha_128);

    const auto &second_error = diag.errors()[1];
    EXPECT_FALSE(second_error.empty());
    bool found_alpha_64 = false;
    for (const auto &line : second_error) {
        if (line.find("64") != std::string::npos) {
            found_alpha_64 = true;
            break;
        }
    }
    EXPECT_TRUE(found_alpha_64);
}
