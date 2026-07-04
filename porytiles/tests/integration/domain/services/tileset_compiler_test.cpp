#include "gtest/gtest.h"

#include <memory>
#include <string>

#include "porytiles/domain/config/artifact_edit_mode.hpp"
#include "porytiles/domain/models/anim_frame.hpp"
#include "porytiles/domain/models/anim_params.hpp"
#include "porytiles/domain/models/animation.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/porymap_tileset_component.hpp"
#include "porytiles/domain/models/porytiles_tileset_component.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/domain/services/tileset_compiler.hpp"
#include "porytiles/infra/services/ascii_tile_printer.hpp"
#include "porytiles/infra/services/color_palette_printer.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

#include "support/mock_domain_config.hpp"

using namespace porytiles;

namespace {

Tileset build_empty_tileset(const std::string &name)
{
    auto porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    return Tileset{name, std::move(porytiles_component), std::move(porymap_component)};
}

// Fills a PixelTile with a single solid color.
PixelTile<Rgba32> make_solid_tile(const Rgba32 &color)
{
    PixelTile<Rgba32> tile;
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, color);
    }
    return tile;
}

// Builds a single-subtile RGBA animation with a key frame and one regular frame, both a solid color.
Animation<Rgba32> make_rgba_animation(const std::string &name, const Rgba32 &color)
{
    AnimParams params;
    params.tile_count(1);

    AnimFrame<Rgba32> key_frame{"key"};
    key_frame.add_tile(make_solid_tile(color));

    AnimFrame<Rgba32> frame0{"0"};
    frame0.add_tile(make_solid_tile(color));

    Animation<Rgba32> anim{name, params};
    anim.key_frame(std::move(key_frame));
    anim.put_frame("0", std::move(frame0));
    return anim;
}

/*
 * Builds a concrete (non-wildcard) Porymap palette whose slot 0 holds the mock extrinsic transparency color
 * (rgba_magenta). Default-constructed Porymap palettes are all-wildcard and make validate_porymap_pal panic during
 * input validation of the paired primary, so a paired-primary fixture must supply concrete palettes.
 */
Palette<Rgba32, pal::max_size> make_concrete_porymap_pal()
{
    Palette<Rgba32, pal::max_size> pal{rgba_black};
    pal.set(0, rgba_magenta);
    return pal;
}

/*
 * Builds a minimal "compiled" primary tileset carrying one animation named anim_name.
 *
 * The Porytiles component holds the RGBA animation (with a key frame) so the secondary's cross-tileset registration
 * passes both stale-primary checks. The Porymap component holds concrete palettes 0..num_pals-1 (so paired-primary
 * palette validation passes) plus a name-only Animation<IndexPixel> of the same name (the other stale-primary check).
 * The animation color is also placed in a non-slot-0 position of Porymap palette 0 so a cross-tileset primary anim
 * whose subtile is unreferenced by any metatile can still resolve its palette via the RGBA fallback path.
 */
Tileset build_compiled_primary_with_anim(
    const std::string &name, const std::string &anim_name, const Rgba32 &color, std::size_t num_pals)
{
    auto porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    porytiles_component->add_anim(make_rgba_animation(anim_name, color));

    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    for (std::size_t i = 0; i < num_pals; ++i) {
        Palette<Rgba32, pal::max_size> pal = make_concrete_porymap_pal();
        if (i == 0) {
            // Place the anim color so the unreferenced primary subtile resolves via RGBA palette fallback.
            pal.set(1, color);
        }
        porymap_component->set_pal(i, pal);
    }
    porymap_component->add_anim(Animation<IndexPixel>{anim_name});

    return Tileset{name, std::move(porytiles_component), std::move(porymap_component)};
}

} // namespace

class TilesetCompilerTestBase : public ::testing::Test {
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

class TilesetCompilerModeComboTests : public TilesetCompilerTestBase {};

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

class TilesetCompilerCrossTilesetAnimTests : public TilesetCompilerTestBase {};

/*
 * Regression test for #328: a secondary animation sharing a name with a paired-primary animation but holding different
 * art must produce a diagnostic, not abort the compiler via the matcher's cross-tileset name panic.
 */
TEST_F(TilesetCompilerCrossTilesetAnimTests, SecondarySameNamedAnimAsPrimaryReportsDiagnostic)
{
    config_.cross_tileset_anim_linking = true;

    auto primary = build_compiled_primary_with_anim("test_primary", "flower", rgba_blue, config_.num_pals_in_primary);

    auto secondary = build_empty_tileset("test_secondary");
    secondary.porytiles_component().add_anim(make_rgba_animation("flower", rgba_red));

    auto compiler = make_compiler();

    auto result = compiler->compile(secondary, true, &primary);

    ASSERT_FALSE(result.has_value());
    const std::string error_text = join_error_chain(result);
    EXPECT_NE(error_text.find("has the same name"), std::string::npos)
        << "Expected 'has the same name' in error, got: " << error_text;
    EXPECT_NE(error_text.find("flower"), std::string::npos) << "Expected 'flower' in error, got: " << error_text;
    EXPECT_NE(error_text.find("Cross-Tileset Animation Linking"), std::string::npos)
        << "Expected config note in error, got: " << error_text;
}

/*
 * With cross-tileset linking disabled, the entire primary-registration block is skipped, so a same-named secondary and
 * primary animation no longer collide and compilation succeeds.
 */
TEST_F(TilesetCompilerCrossTilesetAnimTests, SecondarySameNamedAnimCompilesWhenLinkingDisabled)
{
    config_.cross_tileset_anim_linking = false;

    auto primary = build_compiled_primary_with_anim("test_primary", "flower", rgba_blue, config_.num_pals_in_primary);

    auto secondary = build_empty_tileset("test_secondary");
    secondary.porytiles_component().add_anim(make_rgba_animation("flower", rgba_red));

    auto compiler = make_compiler();

    auto result = compiler->compile(secondary, true, &primary);

    ASSERT_TRUE(result.has_value()) << "Expected compile to succeed, got: " << join_error_chain(result);
}

/*
 * A distinctly-named secondary animation must not trip the same-name check: with cross-tileset linking enabled and no
 * name overlap, the primary animation registers cleanly and compilation succeeds.
 */
TEST_F(TilesetCompilerCrossTilesetAnimTests, DistinctlyNamedSecondaryAnimDoesNotOverFire)
{
    config_.cross_tileset_anim_linking = true;

    auto primary = build_compiled_primary_with_anim("test_primary", "flower", rgba_blue, config_.num_pals_in_primary);

    auto secondary = build_empty_tileset("test_secondary");
    secondary.porytiles_component().add_anim(make_rgba_animation("flower_cave", rgba_red));

    auto compiler = make_compiler();

    auto result = compiler->compile(secondary, true, &primary);

    ASSERT_TRUE(result.has_value()) << "Expected compile to succeed, got: " << join_error_chain(result);
}
