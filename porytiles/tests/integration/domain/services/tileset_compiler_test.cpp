#include "gtest/gtest.h"

#include <memory>
#include <string>

#include "porytiles/domain/config/artifact_edit_mode.hpp"
#include "porytiles/domain/config/frame_linking.hpp"
#include "porytiles/domain/models/anim_frame.hpp"
#include "porytiles/domain/models/anim_override_entry.hpp"
#include "porytiles/domain/models/anim_params.hpp"
#include "porytiles/domain/models/animation.hpp"
#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/metatile.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
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
 *
 * When porymap_anim_tile_count is nonzero, the Porymap animation carries tile_count/tile_offset params so a secondary's
 * primary_references overrides can resolve the primary animation's tile range. It defaults to 0 (name-only) so existing
 * call sites are unaffected.
 */
Tileset build_compiled_primary_with_anim(
    const std::string &name,
    const std::string &anim_name,
    const Rgba32 &color,
    std::size_t num_pals,
    std::size_t porymap_anim_tile_count = 0)
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
    if (porymap_anim_tile_count != 0) {
        AnimParams porymap_params;
        porymap_params.tile_count(porymap_anim_tile_count);
        porymap_params.tile_offset(1);
        porymap_component->add_anim(Animation<IndexPixel>{anim_name, porymap_params});
    }
    else {
        porymap_component->add_anim(Animation<IndexPixel>{anim_name});
    }

    return Tileset{name, std::move(porytiles_component), std::move(porymap_component)};
}

// Builds a primary tileset with exactly one metatile: transparent (extrinsic-transparency-filled) bottom and top
// layers and a solid-color middle layer. The middle-only content infers LayerType::normal, so dual-layerization drops
// the bottom layer.
Tileset build_single_metatile_tileset(const std::string &name, const Rgba32 &middle_color)
{
    auto porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    porytiles_component->bottom(Image<Rgba32>{16, 16, rgba_magenta});
    porytiles_component->middle(Image<Rgba32>{16, 16, middle_color});
    porytiles_component->top(Image<Rgba32>{16, 16, rgba_magenta});

    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    return Tileset{name, std::move(porytiles_component), std::move(porymap_component)};
}

// Adds a manual-frame-linking RGBA animation (tile_count 1) carrying the given override entries to a tileset.
void add_manual_anim(
    Tileset &tileset, const std::string &anim_name, const Rgba32 &color, std::vector<AnimOverrideEntry> overrides)
{
    Animation<Rgba32> anim = make_rgba_animation(anim_name, color);
    AnimParams params = anim.params();
    params.overrides(std::move(overrides));
    anim.params(std::move(params));
    tileset.porytiles_component().add_anim(std::move(anim));
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
            &config_, &schema_, formatter_.get(), diag_.get(), tile_printer_.get(), pal_printer_.get());
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
    // The stock emerald shape: a single behavior field in a 2-byte attribute.
    Schema schema_ = std::move(Schema::create({Field{"behavior", 0x00FF}}, 2)).value();
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

/*
 * Coverage for #330: anim.json override entry validation parity between the manual and primary_references paths. Each
 * override entry is now bounds- and encodability-checked through a shared validator that emits graceful diagnostics
 * instead of panicking (the manual metatile-OOB case previously panicked). These diagnostics do not abort the compile,
 * so every test below expects result.has_value() to remain true and asserts via the buffered diagnostic tag counts.
 */
class TilesetCompilerOverrideValidationTests : public TilesetCompilerTestBase {};

TEST_F(TilesetCompilerOverrideValidationTests, ManualOverrideApplies)
{
    config_.global_frame_linking = FrameLinking::manual;

    auto tileset = build_single_metatile_tileset("test_primary", rgba_green);
    add_manual_anim(
        tileset,
        "anim",
        rgba_red,
        {AnimOverrideEntry{
            .metatile_id = 0,
            .layer = metatile::Layer::middle,
            .subtile = metatile::Subtile::northwest,
            .frame_subtile = 0,
            .pal_index = 0,
            .h_flip = true,
            .v_flip = false}});

    auto compiler = make_compiler();
    auto result = compiler->compile(tileset, false, nullptr);

    ASSERT_TRUE(result.has_value()) << "Expected compile to succeed, got: " << join_error_chain(result);
    EXPECT_FALSE(diag_->error_tag_counts().contains("manual-frame-subtile-oob"));
    EXPECT_FALSE(diag_->error_tag_counts().contains("manual-metatile-oob"));
    EXPECT_FALSE(diag_->error_tag_counts().contains("manual-pal-index-oob"));
    EXPECT_FALSE(diag_->warning_tag_counts().contains("manual-pal-index-unused"));
    EXPECT_FALSE(diag_->warning_tag_counts().contains("manual-dual-layer-drop"));

    // Dual mode with an inferred-normal metatile keeps [middle, top], so the overridden middle/NW entry lands at
    // dual-layer index 0 with the animation's tile offset and the requested horizontal flip.
    const std::size_t tile_offset = result.value()->porytiles_component().anim_for_name("anim").params().tile_offset();
    const auto &bin = result.value()->porymap_component().metatiles_bin();
    ASSERT_FALSE(bin.empty());
    EXPECT_EQ(bin.at(0).tile_index(), tile_offset);
    EXPECT_TRUE(bin.at(0).h_flip());
}

TEST_F(TilesetCompilerOverrideValidationTests, ManualFrameSubtileOobReportsError)
{
    config_.global_frame_linking = FrameLinking::manual;

    auto tileset = build_single_metatile_tileset("test_primary", rgba_green);
    add_manual_anim(
        tileset,
        "anim",
        rgba_red,
        {AnimOverrideEntry{
            .metatile_id = 0,
            .layer = metatile::Layer::middle,
            .subtile = metatile::Subtile::northwest,
            .frame_subtile = 5,
            .pal_index = 0,
            .h_flip = false,
            .v_flip = false}});

    auto compiler = make_compiler();
    auto result = compiler->compile(tileset, false, nullptr);

    ASSERT_TRUE(result.has_value()) << "Expected compile to succeed, got: " << join_error_chain(result);
    ASSERT_TRUE(diag_->error_tag_counts().contains("manual-frame-subtile-oob"));
    EXPECT_EQ(diag_->error_tag_counts().at("manual-frame-subtile-oob"), 1U);
}

TEST_F(TilesetCompilerOverrideValidationTests, ManualMetatileOobReportsError)
{
    config_.global_frame_linking = FrameLinking::manual;

    auto tileset = build_single_metatile_tileset("test_primary", rgba_green);
    add_manual_anim(
        tileset,
        "anim",
        rgba_red,
        {AnimOverrideEntry{
            .metatile_id = 99,
            .layer = metatile::Layer::middle,
            .subtile = metatile::Subtile::northwest,
            .frame_subtile = 0,
            .pal_index = 0,
            .h_flip = false,
            .v_flip = false}});

    auto compiler = make_compiler();
    auto result = compiler->compile(tileset, false, nullptr);

    // Regression: an out-of-range metatile_id previously aborted the compiler via panic().
    ASSERT_TRUE(result.has_value()) << "Expected compile to succeed, got: " << join_error_chain(result);
    ASSERT_TRUE(diag_->error_tag_counts().contains("manual-metatile-oob"));
    EXPECT_EQ(diag_->error_tag_counts().at("manual-metatile-oob"), 1U);
}

TEST_F(TilesetCompilerOverrideValidationTests, ManualPalIndexUnencodableReportsError)
{
    config_.global_frame_linking = FrameLinking::manual;

    auto tileset = build_single_metatile_tileset("test_primary", rgba_green);
    add_manual_anim(
        tileset,
        "anim",
        rgba_red,
        {AnimOverrideEntry{
            .metatile_id = 0,
            .layer = metatile::Layer::middle,
            .subtile = metatile::Subtile::northwest,
            .frame_subtile = 0,
            .pal_index = 16,
            .h_flip = false,
            .v_flip = false}});

    auto compiler = make_compiler();
    auto result = compiler->compile(tileset, false, nullptr);

    ASSERT_TRUE(result.has_value()) << "Expected compile to succeed, got: " << join_error_chain(result);
    ASSERT_TRUE(diag_->error_tag_counts().contains("manual-pal-index-oob"));
    EXPECT_EQ(diag_->error_tag_counts().at("manual-pal-index-oob"), 1U);
}

TEST_F(TilesetCompilerOverrideValidationTests, ManualPalIndexUnconfiguredReportsWarning)
{
    config_.global_frame_linking = FrameLinking::manual;

    auto tileset = build_single_metatile_tileset("test_primary", rgba_green);
    add_manual_anim(
        tileset,
        "anim",
        rgba_red,
        {AnimOverrideEntry{
            .metatile_id = 0,
            .layer = metatile::Layer::middle,
            .subtile = metatile::Subtile::northwest,
            .frame_subtile = 0,
            .pal_index = 14,
            .h_flip = false,
            .v_flip = false}});

    auto compiler = make_compiler();
    auto result = compiler->compile(tileset, false, nullptr);

    ASSERT_TRUE(result.has_value()) << "Expected compile to succeed, got: " << join_error_chain(result);
    ASSERT_TRUE(diag_->warning_tag_counts().contains("manual-pal-index-unused"));
    EXPECT_EQ(diag_->warning_tag_counts().at("manual-pal-index-unused"), 1U);
    EXPECT_TRUE(diag_->error_tag_counts().empty());

    // A pal_index past the configured count is encodable, so the entry still applies.
    const auto &bin = result.value()->porymap_component().metatiles_bin();
    ASSERT_FALSE(bin.empty());
    EXPECT_EQ(bin.at(0).pal_index(), 14U);
}

TEST_F(TilesetCompilerOverrideValidationTests, ManualDualDropReportsWarning)
{
    config_.global_frame_linking = FrameLinking::manual;

    auto tileset = build_single_metatile_tileset("test_primary", rgba_green);
    add_manual_anim(
        tileset,
        "anim",
        rgba_red,
        {AnimOverrideEntry{
            .metatile_id = 0,
            .layer = metatile::Layer::bottom,
            .subtile = metatile::Subtile::northwest,
            .frame_subtile = 0,
            .pal_index = 0,
            .h_flip = false,
            .v_flip = false}});

    auto compiler = make_compiler();
    auto result = compiler->compile(tileset, false, nullptr);

    // The metatile infers LayerType::normal, so dual-layerization drops the bottom layer this override targets.
    ASSERT_TRUE(result.has_value()) << "Expected compile to succeed, got: " << join_error_chain(result);
    ASSERT_TRUE(diag_->warning_tag_counts().contains("manual-dual-layer-drop"));
    EXPECT_EQ(diag_->warning_tag_counts().at("manual-dual-layer-drop"), 1U);
    EXPECT_TRUE(diag_->error_tag_counts().empty());
}

TEST_F(TilesetCompilerOverrideValidationTests, ManualBottomOverrideSurvivesExplicitCoveredLayerType)
{
    // Regression for the effective-layer-type mismatch: this metatile infers 'normal' (which drops the bottom layer),
    // but an explicit 'covered' override keeps the bottom layer. The manual override targets bottom, so it must NOT be
    // rejected. Before the fix, validation used the inferred 'normal' and wrongly warned + dropped the override.
    config_.global_frame_linking = FrameLinking::manual;

    auto tileset = build_single_metatile_tileset("test_primary", rgba_green);

    // Pin metatile 0 to 'covered' via the source Porytiles attribute, exactly as an explicit layerType CSV cell would.
    MetatileAttribute attr_0{};
    attr_0.explicit_layer_type(LayerType::covered);
    tileset.porytiles_component().insert_attribute(0, attr_0);

    add_manual_anim(
        tileset,
        "anim",
        rgba_red,
        {AnimOverrideEntry{
            .metatile_id = 0,
            .layer = metatile::Layer::bottom,
            .subtile = metatile::Subtile::northwest,
            .frame_subtile = 0,
            .pal_index = 0,
            .h_flip = false,
            .v_flip = false}});

    auto compiler = make_compiler();
    auto result = compiler->compile(tileset, false, nullptr);

    // 'covered' drops the top layer, so the bottom override is kept: no drop warning, and the compile is clean.
    ASSERT_TRUE(result.has_value()) << "Expected compile to succeed, got: " << join_error_chain(result);
    EXPECT_FALSE(diag_->warning_tag_counts().contains("manual-dual-layer-drop"));
    EXPECT_TRUE(diag_->error_tag_counts().empty());
}

TEST_F(TilesetCompilerOverrideValidationTests, ManualOverrideOnExplicitlyDroppedLayerStillWarns)
{
    // The complement of the previous test: an explicit 'covered' pin drops the top layer, so an override targeting top
    // must warn even though inference (which would say 'normal', dropping bottom) would have let it through.
    config_.global_frame_linking = FrameLinking::manual;

    auto tileset = build_single_metatile_tileset("test_primary", rgba_green);

    MetatileAttribute attr_0{};
    attr_0.explicit_layer_type(LayerType::covered);
    tileset.porytiles_component().insert_attribute(0, attr_0);

    add_manual_anim(
        tileset,
        "anim",
        rgba_red,
        {AnimOverrideEntry{
            .metatile_id = 0,
            .layer = metatile::Layer::top,
            .subtile = metatile::Subtile::northwest,
            .frame_subtile = 0,
            .pal_index = 0,
            .h_flip = false,
            .v_flip = false}});

    auto compiler = make_compiler();
    auto result = compiler->compile(tileset, false, nullptr);

    ASSERT_TRUE(result.has_value()) << "Expected compile to succeed, got: " << join_error_chain(result);
    ASSERT_TRUE(diag_->warning_tag_counts().contains("manual-dual-layer-drop"));
    EXPECT_EQ(diag_->warning_tag_counts().at("manual-dual-layer-drop"), 1U);
}

TEST_F(TilesetCompilerOverrideValidationTests, ManualBottomOverrideAppliesInTripleMode)
{
    config_.global_frame_linking = FrameLinking::manual;
    config_.num_tiles_per_metatile = 12;

    auto tileset = build_single_metatile_tileset("test_primary", rgba_green);
    add_manual_anim(
        tileset,
        "anim",
        rgba_red,
        {AnimOverrideEntry{
            .metatile_id = 0,
            .layer = metatile::Layer::bottom,
            .subtile = metatile::Subtile::northwest,
            .frame_subtile = 0,
            .pal_index = 0,
            .h_flip = false,
            .v_flip = false}});

    auto compiler = make_compiler();
    auto result = compiler->compile(tileset, false, nullptr);

    // Triple mode keeps all layers, so the bottom/NW override survives at triple-layer index 0.
    ASSERT_TRUE(result.has_value()) << "Expected compile to succeed, got: " << join_error_chain(result);
    EXPECT_FALSE(diag_->warning_tag_counts().contains("manual-dual-layer-drop"));

    const std::size_t tile_offset = result.value()->porytiles_component().anim_for_name("anim").params().tile_offset();
    const auto &bin = result.value()->porymap_component().metatiles_bin();
    ASSERT_FALSE(bin.empty());
    EXPECT_EQ(bin.at(0).tile_index(), tile_offset);
}

TEST_F(TilesetCompilerOverrideValidationTests, PrimaryReferencesFrameSubtileOobReportsError)
{
    auto primary = build_compiled_primary_with_anim(
        "test_primary", "flower", rgba_blue, config_.num_pals_in_primary, /*porymap_anim_tile_count=*/1);

    auto secondary = build_empty_tileset("test_secondary");
    secondary.porytiles_component().primary_anim_overrides(
        {{"flower",
          {AnimOverrideEntry{
              .metatile_id = 0,
              .layer = metatile::Layer::middle,
              .subtile = metatile::Subtile::northwest,
              .frame_subtile = 5,
              .pal_index = 0,
              .h_flip = false,
              .v_flip = false}}}});

    auto compiler = make_compiler();
    auto result = compiler->compile(secondary, true, &primary);

    ASSERT_TRUE(result.has_value()) << "Expected compile to succeed, got: " << join_error_chain(result);
    ASSERT_TRUE(diag_->error_tag_counts().contains("primary-references-frame-subtile-oob"));
    EXPECT_EQ(diag_->error_tag_counts().at("primary-references-frame-subtile-oob"), 1U);
}

TEST_F(TilesetCompilerOverrideValidationTests, PrimaryReferencesMetatileOobReportsError)
{
    auto primary = build_compiled_primary_with_anim(
        "test_primary", "flower", rgba_blue, config_.num_pals_in_primary, /*porymap_anim_tile_count=*/1);

    // A secondary with zero metatiles makes metatile_id 0 out of range.
    auto secondary = build_empty_tileset("test_secondary");
    secondary.porytiles_component().primary_anim_overrides(
        {{"flower",
          {AnimOverrideEntry{
              .metatile_id = 0,
              .layer = metatile::Layer::middle,
              .subtile = metatile::Subtile::northwest,
              .frame_subtile = 0,
              .pal_index = 0,
              .h_flip = false,
              .v_flip = false}}}});

    auto compiler = make_compiler();
    auto result = compiler->compile(secondary, true, &primary);

    ASSERT_TRUE(result.has_value()) << "Expected compile to succeed, got: " << join_error_chain(result);
    ASSERT_TRUE(diag_->error_tag_counts().contains("primary-references-metatile-oob"));
    EXPECT_EQ(diag_->error_tag_counts().at("primary-references-metatile-oob"), 1U);
}

TEST_F(TilesetCompilerOverrideValidationTests, PrimaryReferencesPalIndexUnencodableReportsError)
{
    auto primary = build_compiled_primary_with_anim(
        "test_primary", "flower", rgba_blue, config_.num_pals_in_primary, /*porymap_anim_tile_count=*/1);

    auto secondary = build_single_metatile_tileset("test_secondary", rgba_green);
    secondary.porytiles_component().primary_anim_overrides(
        {{"flower",
          {AnimOverrideEntry{
              .metatile_id = 0,
              .layer = metatile::Layer::middle,
              .subtile = metatile::Subtile::northwest,
              .frame_subtile = 0,
              .pal_index = 16,
              .h_flip = false,
              .v_flip = false}}}});

    auto compiler = make_compiler();
    auto result = compiler->compile(secondary, true, &primary);

    ASSERT_TRUE(result.has_value()) << "Expected compile to succeed, got: " << join_error_chain(result);
    ASSERT_TRUE(diag_->error_tag_counts().contains("primary-references-pal-index-oob"));
    EXPECT_EQ(diag_->error_tag_counts().at("primary-references-pal-index-oob"), 1U);
}

TEST_F(TilesetCompilerOverrideValidationTests, PrimaryReferencesPalIndexUnconfiguredReportsWarning)
{
    auto primary = build_compiled_primary_with_anim(
        "test_primary", "flower", rgba_blue, config_.num_pals_in_primary, /*porymap_anim_tile_count=*/1);

    auto secondary = build_single_metatile_tileset("test_secondary", rgba_green);
    secondary.porytiles_component().primary_anim_overrides(
        {{"flower",
          {AnimOverrideEntry{
              .metatile_id = 0,
              .layer = metatile::Layer::middle,
              .subtile = metatile::Subtile::northwest,
              .frame_subtile = 0,
              .pal_index = 14,
              .h_flip = false,
              .v_flip = false}}}});

    auto compiler = make_compiler();
    auto result = compiler->compile(secondary, true, &primary);

    ASSERT_TRUE(result.has_value()) << "Expected compile to succeed, got: " << join_error_chain(result);
    ASSERT_TRUE(diag_->warning_tag_counts().contains("primary-references-pal-index-unused"));
    EXPECT_EQ(diag_->warning_tag_counts().at("primary-references-pal-index-unused"), 1U);
    EXPECT_TRUE(diag_->error_tag_counts().empty());
}

TEST_F(TilesetCompilerOverrideValidationTests, PrimaryReferencesDualDropReportsWarning)
{
    auto primary = build_compiled_primary_with_anim(
        "test_primary", "flower", rgba_blue, config_.num_pals_in_primary, /*porymap_anim_tile_count=*/1);

    auto secondary = build_single_metatile_tileset("test_secondary", rgba_green);
    secondary.porytiles_component().primary_anim_overrides(
        {{"flower",
          {AnimOverrideEntry{
              .metatile_id = 0,
              .layer = metatile::Layer::bottom,
              .subtile = metatile::Subtile::northwest,
              .frame_subtile = 0,
              .pal_index = 0,
              .h_flip = false,
              .v_flip = false}}}});

    auto compiler = make_compiler();
    auto result = compiler->compile(secondary, true, &primary);

    // The secondary metatile infers LayerType::normal, so dual-layerization drops the bottom layer.
    ASSERT_TRUE(result.has_value()) << "Expected compile to succeed, got: " << join_error_chain(result);
    ASSERT_TRUE(diag_->warning_tag_counts().contains("primary-references-dual-layer-drop"));
    EXPECT_EQ(diag_->warning_tag_counts().at("primary-references-dual-layer-drop"), 1U);
    EXPECT_TRUE(diag_->error_tag_counts().empty());
}
