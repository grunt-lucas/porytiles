#include "gtest/gtest.h"

#include <memory>
#include <string>

#include "porytiles2/domain/config/artifact_edit_mode.hpp"
#include "porytiles2/domain/models/porymap_tileset_component.hpp"
#include "porytiles2/domain/models/porytiles_tileset_component.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/services/tileset_compiler.hpp"
#include "porytiles2/infra/services/ascii_tile_printer.hpp"
#include "porytiles2/infra/services/color_palette_printer.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

#include "support/mock_domain_config.hpp"

using namespace porytiles2;

namespace {

Tileset build_empty_tileset(const std::string &name)
{
    auto porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    return Tileset{name, std::move(porytiles_component), std::move(porymap_component)};
}

} // namespace

class TilesetCompilerModeComboTests : public ::testing::Test {
  protected:
    void SetUp() override
    {
        diag_ = std::make_unique<BufferedUserDiagnostics>();
        formatter_ = std::make_unique<PlainTextFormatter>();
        tile_printer_ = std::make_unique<AsciiTilePrinter>(formatter_.get());
        pal_printer_ = std::make_unique<ColorPalettePrinter>(formatter_.get());
    }

    [[nodiscard]] std::unique_ptr<TilesetCompiler> make_compiler() const
    {
        return std::make_unique<TilesetCompiler>(
            &config_, formatter_.get(), diag_.get(), tile_printer_.get(), pal_printer_.get());
    }

    template <typename T>
    [[nodiscard]] std::string join_error_chain(const ChainableResult<T> &result) const
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
    std::unique_ptr<ColorPalettePrinter> pal_printer_;
    MockDomainConfig config_;
};

TEST_F(TilesetCompilerModeComboTests, PrimaryRejectsPalsOptimizeWithTilesLocked)
{
    config_.tiles_edit_mode = ArtifactEditMode::locked;
    config_.pals_edit_mode = ArtifactEditMode::optimize;

    auto tileset = build_empty_tileset("test_primary");
    auto compiler = make_compiler();

    auto result = compiler->compile(tileset, false, nullptr);

    ASSERT_FALSE(result.has_value());
    const std::string error_text = join_error_chain(result);
    EXPECT_NE(error_text.find("not a valid combination"), std::string::npos)
        << "Expected 'not a valid combination' in error, got: " << error_text;
    EXPECT_NE(error_text.find("test_primary"), std::string::npos)
        << "Expected tileset name in error, got: " << error_text;
}

TEST_F(TilesetCompilerModeComboTests, PrimaryRejectsPalsPatchAsUnimplemented)
{
    config_.pals_edit_mode = ArtifactEditMode::patch;

    auto tileset = build_empty_tileset("test_primary");
    auto compiler = make_compiler();

    auto result = compiler->compile(tileset, false, nullptr);

    ASSERT_FALSE(result.has_value());
    const std::string error_text = join_error_chain(result);
    EXPECT_NE(error_text.find("not yet implemented"), std::string::npos)
        << "Expected 'not yet implemented' in error, got: " << error_text;
    EXPECT_NE(error_text.find("patch"), std::string::npos) << "Expected 'patch' in error, got: " << error_text;
}

TEST_F(TilesetCompilerModeComboTests, SecondaryRejectsTilesNonOptimize)
{
    config_.tiles_edit_mode = ArtifactEditMode::locked;
    config_.pals_edit_mode = ArtifactEditMode::optimize;

    auto tileset = build_empty_tileset("test_secondary");
    auto compiler = make_compiler();

    auto result = compiler->compile(tileset, true, nullptr);

    ASSERT_FALSE(result.has_value());
    const std::string error_text = join_error_chain(result);
    EXPECT_NE(error_text.find("does not yet support tiles edit mode"), std::string::npos)
        << "Expected 'does not yet support tiles edit mode' in error, got: " << error_text;
    EXPECT_NE(error_text.find("test_secondary"), std::string::npos)
        << "Expected tileset name in error, got: " << error_text;
    EXPECT_NE(error_text.find("locked"), std::string::npos) << "Expected 'locked' in error, got: " << error_text;
}

TEST_F(TilesetCompilerModeComboTests, SecondaryRejectsPalsNonOptimize)
{
    config_.tiles_edit_mode = ArtifactEditMode::optimize;
    config_.pals_edit_mode = ArtifactEditMode::locked;

    auto tileset = build_empty_tileset("test_secondary");
    auto compiler = make_compiler();

    auto result = compiler->compile(tileset, true, nullptr);

    ASSERT_FALSE(result.has_value());
    const std::string error_text = join_error_chain(result);
    EXPECT_NE(error_text.find("does not yet support pals edit mode"), std::string::npos)
        << "Expected 'does not yet support pals edit mode' in error, got: " << error_text;
    EXPECT_NE(error_text.find("test_secondary"), std::string::npos)
        << "Expected tileset name in error, got: " << error_text;
    EXPECT_NE(error_text.find("locked"), std::string::npos) << "Expected 'locked' in error, got: " << error_text;
}
