#include "gtest/gtest.h"

#include <array>
#include <optional>
#include <vector>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/packing/models/palette_hint.hpp"
#include "porytiles2/domain/services/palette_validator.hpp"
#include "porytiles2/infra/config/default_provider.hpp"
#include "porytiles2/infra/config/lazy_layered_config.hpp"
#include "porytiles2/infra/services/color_palette_printer.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

using namespace porytiles2;

class PaletteValidatorTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        formatter_ = std::make_unique<PlainTextFormatter>();
        diag_ = std::make_unique<BufferedUserDiagnostics>();
        pal_printer_ = std::make_unique<ColorPalettePrinter>(formatter_.get());
        std::vector<std::unique_ptr<ConfigProvider>> providers{};
        providers.push_back(std::make_unique<DefaultProvider>());
        config_ = std::make_unique<LazyLayeredConfig>(formatter_.get(), std::move(providers));
        validator_ = std::make_unique<PaletteValidator>(
            formatter_.get(), diag_.get(), pal_printer_.get(), config_.get(), "test_tileset");
    }

    // Helper to create a valid 16-color palette with slot 0 = magenta (extrinsic transparency)
    static Palette<Rgba32, pal::max_size> create_valid_palette()
    {
        Palette<Rgba32, pal::max_size> pal{};
        pal.set(0, rgba_magenta); // Slot 0 matches extrinsic transparency
        for (std::size_t i = 1; i < pal::max_size; ++i) {
            pal.set(i, Rgba32{static_cast<std::uint8_t>(i * 10), 100, 100, Rgba32::alpha_opaque});
        }
        return pal;
    }

    // Helper to create valid porymap palettes array
    static std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> create_valid_porymap_pals()
    {
        std::array<Palette<Rgba32, pal::max_size>, pal::num_pals> pals{};
        for (std::size_t i = 0; i < pal::num_pals; ++i) {
            pals[i] = create_valid_palette();
        }
        return pals;
    }

    // Helper to create empty porytiles palettes array
    static std::array<std::optional<Palette<Rgba32, pal::max_size>>, pal::num_pals> create_empty_porytiles_pals()
    {
        return {};
    }

    // Helper to create empty hints vector
    static std::vector<PaletteHint> create_empty_hints()
    {
        return {};
    }

    std::unique_ptr<PlainTextFormatter> formatter_;
    std::unique_ptr<BufferedUserDiagnostics> diag_;
    std::unique_ptr<ColorPalettePrinter> pal_printer_;
    std::unique_ptr<LazyLayeredConfig> config_;
    std::unique_ptr<PaletteValidator> validator_;
};

// ==================== Full Validation Tests ====================

TEST_F(PaletteValidatorTest, ValidatePrimary_AllValid_ReturnsSuccess)
{
    auto porymap_pals = create_valid_porymap_pals();
    auto porytiles_pals = create_empty_porytiles_pals();
    auto hints = create_empty_hints();

    auto result = validator_->validate_primary(porymap_pals, porytiles_pals, hints);

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(diag_->errors().empty());
    EXPECT_TRUE(diag_->warnings().empty());
}

TEST_F(PaletteValidatorTest, ValidatePrimary_WithValidPorytilesPal_ReturnsSuccess)
{
    auto porymap_pals = create_valid_porymap_pals();
    auto porytiles_pals = create_empty_porytiles_pals();
    porytiles_pals[0] = create_valid_palette();
    auto hints = create_empty_hints();

    auto result = validator_->validate_primary(porymap_pals, porytiles_pals, hints);

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(diag_->errors().empty());
}

TEST_F(PaletteValidatorTest, ValidatePrimary_WithValidHint_ReturnsSuccess)
{
    auto porymap_pals = create_valid_porymap_pals();
    auto porytiles_pals = create_empty_porytiles_pals();

    std::vector<PaletteHint> hints{};
    Palette<Rgba32> hint_pal{};
    hint_pal.add(Rgba32{100, 50, 50, Rgba32::alpha_opaque});
    hint_pal.add(Rgba32{50, 100, 50, Rgba32::alpha_opaque});
    hints.emplace_back("valid_hint", hint_pal);

    auto result = validator_->validate_primary(porymap_pals, porytiles_pals, hints);

    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(diag_->errors().empty());
}

// ==================== Porytiles Palette Validation Tests ====================

TEST_F(PaletteValidatorTest, ValidatePrimary_PorytilesPalWithWildcards_ReturnsSuccess)
{
    auto porymap_pals = create_valid_porymap_pals();
    auto porytiles_pals = create_empty_porytiles_pals();
    auto hints = create_empty_hints();

    // Create porytiles palette with wildcards (slots 10-15 remain wildcards)
    Palette<Rgba32, pal::max_size> pal{};
    pal.set(0, rgba_magenta);
    for (std::size_t i = 1; i < 10; ++i) {
        pal.set(i, Rgba32{static_cast<std::uint8_t>(i * 10), 100, 100, Rgba32::alpha_opaque});
    }
    porytiles_pals[0] = pal;

    auto result = validator_->validate_primary(porymap_pals, porytiles_pals, hints);

    // Wildcards are allowed in porytiles palettes
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(diag_->errors().empty());
}

TEST_F(PaletteValidatorTest, ValidatePrimary_PorytilesPalTransparencyInNonSlot0_ReturnsError)
{
    auto porymap_pals = create_valid_porymap_pals();
    auto porytiles_pals = create_empty_porytiles_pals();
    auto hints = create_empty_hints();

    auto pal = create_valid_palette();
    pal.set(5, rgba_magenta); // Transparency in non-slot-0
    porytiles_pals[0] = pal;

    auto result = validator_->validate_primary(porymap_pals, porytiles_pals, hints);

    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(diag_->errors().empty());

    // Check that the error mentions the slot
    bool found_slot_5 = false;
    for (const auto &error : diag_->errors()) {
        for (const auto &line : error) {
            if (line.find("5") != std::string::npos) {
                found_slot_5 = true;
                break;
            }
        }
    }
    EXPECT_TRUE(found_slot_5);
}

TEST_F(PaletteValidatorTest, ValidatePrimary_PorytilesPalSlot0Mismatch_EmitsWarning)
{
    auto porymap_pals = create_valid_porymap_pals();
    auto porytiles_pals = create_empty_porytiles_pals();
    auto hints = create_empty_hints();

    auto pal = create_valid_palette();
    pal.set(0, Rgba32{100, 100, 100, Rgba32::alpha_opaque}); // Different from magenta
    porytiles_pals[0] = pal;

    auto result = validator_->validate_primary(porymap_pals, porytiles_pals, hints);

    // Should succeed but with a warning
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(diag_->errors().empty());
    EXPECT_FALSE(diag_->warnings().empty());
}

TEST_F(PaletteValidatorTest, ValidatePrimary_MultipleInvalidPorytilesPals_ReturnsAllErrors)
{
    auto porymap_pals = create_valid_porymap_pals();
    auto porytiles_pals = create_empty_porytiles_pals();
    auto hints = create_empty_hints();

    // Create two invalid palettes with extrinsic transparency in non-slot-0 positions
    Palette<Rgba32, pal::max_size> pal1 = create_valid_palette();
    pal1.set(5, rgba_magenta);
    porytiles_pals[0] = pal1;

    Palette<Rgba32, pal::max_size> pal2 = create_valid_palette();
    pal2.set(10, rgba_magenta);
    porytiles_pals[3] = pal2;

    auto result = validator_->validate_primary(porymap_pals, porytiles_pals, hints);

    EXPECT_FALSE(result.has_value());
    // Both palettes should generate errors
    EXPECT_GE(diag_->errors().size(), 2);
}

// ==================== Porymap Palette Validation Tests ====================

TEST_F(PaletteValidatorTest, ValidatePrimary_PorymapTransparencyInNonSlot0_ReturnsError)
{
    auto porymap_pals = create_valid_porymap_pals();
    porymap_pals[0].set(5, rgba_magenta); // Transparency in non-slot-0
    auto porytiles_pals = create_empty_porytiles_pals();
    auto hints = create_empty_hints();

    auto result = validator_->validate_primary(porymap_pals, porytiles_pals, hints);

    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(diag_->errors().empty());
}

TEST_F(PaletteValidatorTest, ValidatePrimary_PorymapSlot0Mismatch_EmitsWarning)
{
    auto porymap_pals = create_valid_porymap_pals();
    porymap_pals[0].set(0, Rgba32{100, 100, 100, Rgba32::alpha_opaque}); // Different from magenta
    auto porytiles_pals = create_empty_porytiles_pals();
    auto hints = create_empty_hints();

    auto result = validator_->validate_primary(porymap_pals, porytiles_pals, hints);

    // Should succeed but with a warning
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(diag_->errors().empty());
    EXPECT_FALSE(diag_->warnings().empty());
}

TEST_F(PaletteValidatorTest, ValidatePrimary_MultiplePorymapErrors_ReturnsAllErrors)
{
    auto porymap_pals = create_valid_porymap_pals();
    porymap_pals[0].set(5, rgba_magenta);
    porymap_pals[3].set(10, rgba_magenta);
    auto porytiles_pals = create_empty_porytiles_pals();
    auto hints = create_empty_hints();

    auto result = validator_->validate_primary(porymap_pals, porytiles_pals, hints);

    EXPECT_FALSE(result.has_value());
    // Both palettes should generate errors
    EXPECT_GE(diag_->errors().size(), 2);
}

// ==================== Hint Validation Tests ====================

TEST_F(PaletteValidatorTest, ValidatePrimary_HintDuplicateColors_ReturnsError)
{
    auto porymap_pals = create_valid_porymap_pals();
    auto porytiles_pals = create_empty_porytiles_pals();

    std::vector<PaletteHint> hints{};
    Palette<Rgba32> pal{};
    Rgba32 duplicate_color{100, 50, 50, Rgba32::alpha_opaque};
    pal.add(duplicate_color);
    pal.add(Rgba32{50, 100, 50, Rgba32::alpha_opaque});
    pal.add(duplicate_color); // Duplicate!
    hints.emplace_back("test_hint", pal);

    auto result = validator_->validate_primary(porymap_pals, porytiles_pals, hints);

    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(diag_->errors().empty());

    // Check error mentions "duplicate"
    bool found_duplicate = false;
    for (const auto &error : diag_->errors()) {
        for (const auto &line : error) {
            if (line.find("duplicate") != std::string::npos || line.find("Duplicate") != std::string::npos) {
                found_duplicate = true;
                break;
            }
        }
    }
    EXPECT_TRUE(found_duplicate);
}

TEST_F(PaletteValidatorTest, ValidatePrimary_HintTransparencyColor_ReturnsError)
{
    auto porymap_pals = create_valid_porymap_pals();
    auto porytiles_pals = create_empty_porytiles_pals();

    std::vector<PaletteHint> hints{};
    Palette<Rgba32> pal{};
    pal.add(Rgba32{100, 50, 50, Rgba32::alpha_opaque});
    pal.add(rgba_magenta); // Extrinsic transparency - not allowed in hints
    pal.add(Rgba32{50, 50, 100, Rgba32::alpha_opaque});
    hints.emplace_back("test_hint", pal);

    auto result = validator_->validate_primary(porymap_pals, porytiles_pals, hints);

    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(diag_->errors().empty());

    // Check error mentions "transparency"
    bool found_transparency = false;
    for (const auto &error : diag_->errors()) {
        for (const auto &line : error) {
            if (line.find("transparency") != std::string::npos || line.find("Transparency") != std::string::npos) {
                found_transparency = true;
                break;
            }
        }
    }
    EXPECT_TRUE(found_transparency);
}

TEST_F(PaletteValidatorTest, ValidatePrimary_MultipleInvalidHints_ReturnsAllErrors)
{
    auto porymap_pals = create_valid_porymap_pals();
    auto porytiles_pals = create_empty_porytiles_pals();

    std::vector<PaletteHint> hints{};

    // Hint with duplicate colors
    Palette<Rgba32> pal1{};
    Rgba32 dup{100, 50, 50, Rgba32::alpha_opaque};
    pal1.add(dup);
    pal1.add(dup);
    hints.emplace_back("hint_with_dups", pal1);

    // Hint with extrinsic transparency
    Palette<Rgba32> pal2{};
    pal2.add(rgba_magenta);
    hints.emplace_back("hint_with_transparency", pal2);

    auto result = validator_->validate_primary(porymap_pals, porytiles_pals, hints);

    EXPECT_FALSE(result.has_value());
    // Both hints should generate errors
    EXPECT_GE(diag_->errors().size(), 2);
}

// ==================== Combined Error Tests ====================

TEST_F(PaletteValidatorTest, ValidatePrimary_ErrorsInAllCategories_ReturnsAllErrors)
{
    auto porymap_pals = create_valid_porymap_pals();
    porymap_pals[0].set(5, rgba_magenta); // Porymap error

    auto porytiles_pals = create_empty_porytiles_pals();
    auto porytiles_pal = create_valid_palette();
    porytiles_pal.set(6, rgba_magenta); // Porytiles palette error
    porytiles_pals[1] = porytiles_pal;

    std::vector<PaletteHint> hints{};
    Palette<Rgba32> hint_pal{};
    hint_pal.add(rgba_magenta); // Hint error
    hints.emplace_back("bad_hint", hint_pal);

    auto result = validator_->validate_primary(porymap_pals, porytiles_pals, hints);

    EXPECT_FALSE(result.has_value());
    // Should have errors from all three categories
    EXPECT_GE(diag_->errors().size(), 3);
}
