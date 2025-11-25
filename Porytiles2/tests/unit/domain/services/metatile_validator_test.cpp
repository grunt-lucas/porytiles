#include "gtest/gtest.h"

#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/services/metatile_validator.hpp"
#include "porytiles2/infra/services/ascii_tile_printer.hpp"
#include "porytiles2/infra/services/color_palette_printer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/config/config_scope_type.hpp"
#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

namespace porytiles2 {

class MockDomainConfig : public DomainConfig {
  protected:
    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    num_tiles_primary_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue<std::size_t>{512, "num_tiles_primary", "default value", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    num_tiles_total_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue<std::size_t>{1024, "num_tiles_total", "default value", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    num_metatiles_primary_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue<std::size_t>{512, "num_metatiles_primary", "default value", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    num_metatiles_total_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue<std::size_t>{1024, "num_metatiles_total", "default value", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    num_pals_primary_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue<std::size_t>{6, "num_pals_primary", "default value", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    num_pals_total_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue<std::size_t>{13, "num_pals_total", "default value", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    max_map_data_size_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue<std::size_t>{10240, "max_map_data_size", "default value", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<std::size_t>>
    num_tiles_per_metatile_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue<std::size_t>{8, "num_tiles_per_metatile", "default value", {}};
    }

    [[nodiscard]] ChainableResult<ConfigValue<Rgba32>>
    extrinsic_transparency_raw(ConfigScopeType, const std::string &) const override
    {
        return ConfigValue<Rgba32>{rgba_magenta, "extrinsic_transparency", "default value", {}};
    }
};

// Friend test class to allow testing private methods
class MetatileValidatorTestAccess {
  public:
    static ChainableResult<void>
    validate_alpha_channels(const MetatileValidator &validator, const std::vector<Metatile<Rgba32>> &metatiles)
    {
        return validator.validate_alpha_channels(metatiles);
    }
};

} // namespace porytiles2

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

    // Create a metatile and set the tiles into it
    Metatile<Rgba32> metatile{};
    metatile.set_bottom(0, tile1);
    metatile.set_bottom(1, tile2);

    std::vector metatiles = {metatile};

    PlainTextFormatter formatter{};
    BufferedUserDiagnostics diag{};
    AsciiTilePrinter tile_printer{&formatter, rgba_magenta};
    ColorPalettePrinter palette_printer{&formatter};
    MockDomainConfig config{};
    MetatileValidator validator{&formatter, &diag, &tile_printer, &palette_printer, &config, "test_tileset"};

    auto result = MetatileValidatorTestAccess::validate_alpha_channels(validator, metatiles);

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

    // Create a metatile and set the tiles into it
    Metatile<Rgba32> metatile{};
    metatile.set_bottom(0, tile1);
    metatile.set_bottom(1, tile2);

    std::vector<Metatile<Rgba32>> metatiles = {metatile};

    PlainTextFormatter formatter{};
    BufferedUserDiagnostics diag{};
    AsciiTilePrinter tile_printer{&formatter, rgba_magenta};
    ColorPalettePrinter palette_printer{&formatter};
    MockDomainConfig config{};
    MetatileValidator validator{&formatter, &diag, &tile_printer, &palette_printer, &config, "test_tileset"};

    auto result = MetatileValidatorTestAccess::validate_alpha_channels(validator, metatiles);

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
