#include "porytiles/domain/services/primary_tileset_decompiler.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "porytiles/domain/config/import_transparency_mode.hpp"
#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/layer.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/porymap_tileset_component.hpp"
#include "porytiles/domain/models/porytiles_tileset_component.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/tilemap_entry.hpp"
#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/infra/services/ascii_tile_printer.hpp"
#include "porytiles/infra/services/color_palette_printer.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

#include "support/mock_domain_config.hpp"

using namespace porytiles;

namespace {

// Builds a minimal Porymap component with two metatiles whose bin layer types are covered (0) and normal (1). Every
// tilemap entry is transparent (tile 0, palette 0), which is enough for the decompile pipeline to run while keeping the
// layer_type round-trip and the transparency representation the only interesting variables.
std::unique_ptr<PorymapTilesetComponent> make_two_metatile_porymap(const LayerMode layer_mode = LayerMode::dual)
{
    auto porymap = std::make_unique<PorymapTilesetComponent>();

    MetatileAttribute attribute_0{};
    attribute_0.layer_type(LayerType::covered);
    MetatileAttribute attribute_1{};
    attribute_1.layer_type(LayerType::normal);
    porymap->push_back_attribute(attribute_0);
    porymap->push_back_attribute(attribute_1);

    const std::size_t entries_per_metatile =
        layer_mode == LayerMode::dual ? metatile::entries_per_metatile_dual : metatile::entries_per_metatile_triple;
    std::vector<TilemapEntry> entries(2 * entries_per_metatile, TilemapEntry{0, 0, false, false});
    porymap->metatiles_bin(entries);

    // A single-tile tiles.png covers the sole referenced tile index (0).
    porymap->tiles_png(Image<IndexPixel>{tile::side_length_pix, tile::side_length_pix});

    return porymap;
}

class PrimaryTilesetDecompilerRoundTripTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        diag_ = std::make_unique<BufferedUserDiagnostics>();
        formatter_ = std::make_unique<PlainTextFormatter>();
        tile_printer_ = std::make_unique<AsciiTilePrinter>(formatter_.get());
        palette_printer_ = std::make_unique<ColorPalettePrinter>(formatter_.get());
    }

    [[nodiscard]] PrimaryTilesetDecompiler make_decompiler() const
    {
        return PrimaryTilesetDecompiler{
            &config_, formatter_.get(), diag_.get(), tile_printer_.get(), palette_printer_.get()};
    }

    [[nodiscard]] std::string join_error_chain(const auto &result) const
    {
        std::string text;
        for (const auto &err : result.chain()) {
            for (const auto &line : err->details(*formatter_)) {
                text += line;
                text += '\n';
            }
        }
        return text;
    }

    std::unique_ptr<BufferedUserDiagnostics> diag_;
    std::unique_ptr<PlainTextFormatter> formatter_;
    std::unique_ptr<AsciiTilePrinter> tile_printer_;
    std::unique_ptr<ColorPalettePrinter> palette_printer_;
    MockDomainConfig config_;
};

// no_csv (a fresh import): every row is pinned from the bin.
TEST_F(PrimaryTilesetDecompilerRoundTripTest, NoCsvPinsEveryRowFromBin)
{
    auto porytiles = std::make_unique<PorytilesTilesetComponent>();
    porytiles->prior_pin_column_state(FieldRole::layer_type, PriorPinColumnState::no_csv);
    Tileset input{"test_primary", std::move(porytiles), make_two_metatile_porymap()};

    auto result = make_decompiler().decompile(input);
    ASSERT_TRUE(result.has_value()) << join_error_chain(result);

    const auto &out = result.value()->porytiles_component();
    ASSERT_TRUE(out.get_attribute(0).has_value());
    ASSERT_TRUE(out.get_attribute(0)->explicit_layer_type().has_value());
    EXPECT_EQ(out.get_attribute(0)->explicit_layer_type().value(), LayerType::covered);
    ASSERT_TRUE(out.get_attribute(1)->explicit_layer_type().has_value());
    EXPECT_EQ(out.get_attribute(1)->explicit_layer_type().value(), LayerType::normal);
}

// column_absent (a decompile whose CSV lacked the pin column): every row is pinned from the bin, same as no_csv.
TEST_F(PrimaryTilesetDecompilerRoundTripTest, ColumnAbsentPinsEveryRowFromBin)
{
    auto porytiles = std::make_unique<PorytilesTilesetComponent>();
    porytiles->prior_pin_column_state(FieldRole::layer_type, PriorPinColumnState::column_absent);
    Tileset input{"test_primary", std::move(porytiles), make_two_metatile_porymap()};

    auto result = make_decompiler().decompile(input);
    ASSERT_TRUE(result.has_value()) << join_error_chain(result);

    const auto &out = result.value()->porytiles_component();
    ASSERT_TRUE(out.get_attribute(0)->explicit_layer_type().has_value());
    EXPECT_EQ(out.get_attribute(0)->explicit_layer_type().value(), LayerType::covered);
    ASSERT_TRUE(out.get_attribute(1)->explicit_layer_type().has_value());
    EXPECT_EQ(out.get_attribute(1)->explicit_layer_type().value(), LayerType::normal);
}

// column_present: metatile 0 was pinned in the prior CSV (stays pinned, value refreshed from the bin); metatile 1 had a
// blank prior cell (stays unpinned).
TEST_F(PrimaryTilesetDecompilerRoundTripTest, ColumnPresentPreservesPriorPerRowPinState)
{
    auto porytiles = std::make_unique<PorytilesTilesetComponent>();
    porytiles->prior_pin_column_state(FieldRole::layer_type, PriorPinColumnState::column_present);
    // Prior metatile 0 was pinned (to split, deliberately different from the bin's covered); metatile 1 was blank.
    MetatileAttribute prior_0{};
    prior_0.explicit_layer_type(LayerType::split);
    porytiles->insert_attribute(0, prior_0);
    MetatileAttribute prior_1{};
    prior_1.layer_type(LayerType::normal); // unpinned
    porytiles->insert_attribute(1, prior_1);
    Tileset input{"test_primary", std::move(porytiles), make_two_metatile_porymap()};

    auto result = make_decompiler().decompile(input);
    ASSERT_TRUE(result.has_value()) << join_error_chain(result);

    const auto &out = result.value()->porytiles_component();
    // Metatile 0 stays pinned, but its value is refreshed from the bin (covered), not the prior pin (split).
    ASSERT_TRUE(out.get_attribute(0)->explicit_layer_type().has_value());
    EXPECT_EQ(out.get_attribute(0)->explicit_layer_type().value(), LayerType::covered);
    // Metatile 1's prior blank cell stays unpinned.
    EXPECT_FALSE(out.get_attribute(1)->explicit_layer_type().has_value());
}

// column_present with a row absent from the prior CSV (metatile 1 has no prior attribute): stays unpinned.
TEST_F(PrimaryTilesetDecompilerRoundTripTest, ColumnPresentAbsentPriorRowStaysUnpinned)
{
    auto porytiles = std::make_unique<PorytilesTilesetComponent>();
    porytiles->prior_pin_column_state(FieldRole::layer_type, PriorPinColumnState::column_present);
    // Only metatile 0 has a prior attribute (pinned); metatile 1 is absent from the prior CSV.
    MetatileAttribute prior_0{};
    prior_0.explicit_layer_type(LayerType::covered);
    porytiles->insert_attribute(0, prior_0);
    Tileset input{"test_primary", std::move(porytiles), make_two_metatile_porymap()};

    auto result = make_decompiler().decompile(input);
    ASSERT_TRUE(result.has_value()) << join_error_chain(result);

    const auto &out = result.value()->porytiles_component();
    ASSERT_TRUE(out.get_attribute(0)->explicit_layer_type().has_value());
    EXPECT_EQ(out.get_attribute(0)->explicit_layer_type().value(), LayerType::covered);
    EXPECT_FALSE(out.get_attribute(1)->explicit_layer_type().has_value());
}

// Every pixel of one metatile's 16x16 block in a decompiled layer image must equal the expected color, alpha included
// (Rgba32 equality compares all four channels). Layer images place metatile i at columns [i * 16, i * 16 + 16) of the
// first metatile row.
[[nodiscard]] testing::AssertionResult
metatile_block_is(const Image<Rgba32> &layer_image, std::size_t metatile_idx, const Rgba32 &expected)
{
    const std::size_t first_col = metatile_idx * metatile::side_length_pix;
    for (std::size_t row = 0; row < metatile::side_length_pix; ++row) {
        for (std::size_t col = first_col; col < first_col + metatile::side_length_pix; ++col) {
            const Rgba32 actual = layer_image.at(row, col);
            if (actual != expected) {
                return testing::AssertionFailure() << "metatile " << metatile_idx << " pixel (" << row << ", " << col
                                                   << ") is " << actual << ", expected " << expected;
            }
        }
    }
    return testing::AssertionSuccess();
}

class PrimaryTilesetDecompilerImportTransparencyTest : public PrimaryTilesetDecompilerRoundTripTest {
  protected:
    [[nodiscard]] std::unique_ptr<Tileset> decompile(const LayerMode layer_mode)
    {
        Tileset input{
            "test_primary", std::make_unique<PorytilesTilesetComponent>(), make_two_metatile_porymap(layer_mode)};
        auto result = make_decompiler().decompile(input);
        EXPECT_TRUE(result.has_value()) << join_error_chain(result);
        return std::move(result).value();
    }

    const Rgba32 alpha_zero_{};
};

TEST_F(PrimaryTilesetDecompilerImportTransparencyTest, ExtrinsicWritesExtrinsicColorOnEveryLayer)
{
    config_.import_transparency = ImportTransparencyMode::extrinsic;

    const auto out = decompile(LayerMode::dual);

    const auto &porytiles = out->porytiles_component();
    for (const std::size_t metatile_idx : {0, 1}) {
        EXPECT_TRUE(metatile_block_is(porytiles.bottom(), metatile_idx, rgba_magenta));
        EXPECT_TRUE(metatile_block_is(porytiles.middle(), metatile_idx, rgba_magenta));
        EXPECT_TRUE(metatile_block_is(porytiles.top(), metatile_idx, rgba_magenta));
    }
}

TEST_F(PrimaryTilesetDecompilerImportTransparencyTest, AlphaWritesAlphaZeroOnEveryLayer)
{
    config_.import_transparency = ImportTransparencyMode::alpha;

    const auto out = decompile(LayerMode::dual);

    const auto &porytiles = out->porytiles_component();
    for (const std::size_t metatile_idx : {0, 1}) {
        EXPECT_TRUE(metatile_block_is(porytiles.bottom(), metatile_idx, alpha_zero_));
        EXPECT_TRUE(metatile_block_is(porytiles.middle(), metatile_idx, alpha_zero_));
        EXPECT_TRUE(metatile_block_is(porytiles.top(), metatile_idx, alpha_zero_));
    }
}

// Dual-layer input: metatile 0 is covered (top synthesized), metatile 1 is normal (bottom synthesized). Only the
// synthesized layer of each metatile is alpha 0.
TEST_F(PrimaryTilesetDecompilerImportTransparencyTest, MixedDualWritesAlphaZeroOnSynthesizedLayerOnly)
{
    config_.import_transparency = ImportTransparencyMode::mixed;

    const auto out = decompile(LayerMode::dual);

    const auto &porytiles = out->porytiles_component();
    EXPECT_TRUE(metatile_block_is(porytiles.bottom(), 0, rgba_magenta));
    EXPECT_TRUE(metatile_block_is(porytiles.middle(), 0, rgba_magenta));
    EXPECT_TRUE(metatile_block_is(porytiles.top(), 0, alpha_zero_));

    EXPECT_TRUE(metatile_block_is(porytiles.bottom(), 1, alpha_zero_));
    EXPECT_TRUE(metatile_block_is(porytiles.middle(), 1, rgba_magenta));
    EXPECT_TRUE(metatile_block_is(porytiles.top(), 1, rgba_magenta));
}

TEST_F(PrimaryTilesetDecompilerImportTransparencyTest, MixedTripleWritesExtrinsicColorOnEveryLayer)
{
    config_.import_transparency = ImportTransparencyMode::mixed;

    const auto out = decompile(LayerMode::triple);

    const auto &porytiles = out->porytiles_component();
    for (const std::size_t metatile_idx : {0, 1}) {
        EXPECT_TRUE(metatile_block_is(porytiles.bottom(), metatile_idx, rgba_magenta));
        EXPECT_TRUE(metatile_block_is(porytiles.middle(), metatile_idx, rgba_magenta));
        EXPECT_TRUE(metatile_block_is(porytiles.top(), metatile_idx, rgba_magenta));
    }
}

} // namespace
