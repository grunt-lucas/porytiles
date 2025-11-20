#include "gtest/gtest.h"

#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/pixel_tile.hpp"
#include "porytiles2/domain/models/porymap_tileset_component.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/domain/services/layer_mode_converter.hpp"
#include "porytiles2/infra/services/ascii_tile_printer.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/stderr_styled_user_diagnostics.hpp"

using namespace porytiles2;

namespace {

// Helper function to create a non-transparent TilemapEntry for testing
TilemapEntry create_test_entry(unsigned int tile_index, unsigned int pal_index = 0)
{
    return TilemapEntry{tile_index, pal_index, false, false};
}

// Helper function to create a dual-layer component with one metatile
PorymapTilesetComponent create_dual_layer_component_single_metatile(LayerType layer_type)
{
    PorymapTilesetComponent component;

    // Add 8 tilemap entries (dual layer metatile)
    for (unsigned int i = 1; i <= metatile::entries_per_metatile_dual; ++i) {
        component.push_back_tilemap_entry(create_test_entry(i));
    }

    // Add one metatile attribute
    component.push_back_attribute(MetatileAttribute{layer_type, 0});

    return component;
}

// Helper function to create a triple-layer component with one metatile
PorymapTilesetComponent create_triple_layer_component_single_metatile(LayerType layer_type)
{
    PorymapTilesetComponent component;

    // Add 12 tilemap entries (triple layer metatile)
    for (unsigned int i = 1; i <= metatile::entries_per_metatile_triple; ++i) {
        component.push_back_tilemap_entry(create_test_entry(i));
    }

    // Add one metatile attribute
    component.push_back_attribute(MetatileAttribute{layer_type, 0});

    return component;
}

// Helper function to create a non-transparent RGBA tile
PixelTile<Rgba32> create_nontransparent_rgba_tile()
{
    PixelTile<Rgba32> tile{};
    constexpr Rgba32 red{255, 0, 0, 255};
    for (std::size_t i = 0; i < tile::size_pix; ++i) {
        tile.set(i, red);
    }
    return tile;
}

// Helper function to create a Metatile<Rgba32> with specified LayerType
// Uses magenta as extrinsic transparency (matches the converter's behavior)
Metatile<Rgba32> create_metatile_with_layer_type(LayerType layer_type)
{
    Metatile<Rgba32> metatile{};

    switch (layer_type) {
    case LayerType::normal:
        // Content in middle and/or top layers only
        metatile.set_middle(0, create_nontransparent_rgba_tile());
        break;

    case LayerType::covered:
        // Content in bottom and/or middle layers only
        metatile.set_bottom(0, create_nontransparent_rgba_tile());
        break;

    case LayerType::split:
        // Content in bottom and top layers only
        metatile.set_bottom(0, create_nontransparent_rgba_tile());
        metatile.set_top(0, create_nontransparent_rgba_tile());
        break;
    }

    return metatile;
}

} // namespace

class LayerModeConverterTests : public ::testing::Test {
  protected:
    void SetUp() override
    {
        format_ = std::make_unique<PlainTextFormatter>();
        diag_ = std::make_unique<StderrStyledUserDiagnostics>(format_.get());
        tile_printer_ = std::make_unique<AsciiTilePrinter>(format_.get(), rgba_magenta);
        converter_ = std::make_unique<LayerModeConverter>(format_.get(), diag_.get(), tile_printer_.get());
    }

    std::unique_ptr<PlainTextFormatter> format_;
    std::unique_ptr<StderrStyledUserDiagnostics> diag_;
    std::unique_ptr<AsciiTilePrinter> tile_printer_;
    std::unique_ptr<LayerModeConverter> converter_;
};

TEST_F(LayerModeConverterTests, TripleLayerizeNoOpForTripleLayerComponent)
{
    // Create a component that's already in triple layer mode
    auto component = create_triple_layer_component_single_metatile(LayerType::normal);

    // Triple layerize should be a no-op
    auto result = converter_->triple_layerize(component);

    ASSERT_TRUE(result.has_value());
    const auto &entries = result.value();

    // Should return the same entries
    EXPECT_EQ(entries.size(), metatile::entries_per_metatile_triple);
}

TEST_F(LayerModeConverterTests, TripleLayerizeNormalLayerTypeInsertsTransparentAtStart)
{
    // Create a dual-layer component with LayerType::normal
    auto component = create_dual_layer_component_single_metatile(LayerType::normal);

    auto result = converter_->triple_layerize(component);

    ASSERT_TRUE(result.has_value());
    const auto &entries = result.value();

    // Should have 12 entries total (4 transparent + 8 original)
    ASSERT_EQ(entries.size(), metatile::entries_per_metatile_triple);

    // First 4 entries should be transparent
    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_TRUE(entries[i].is_transparent()) << "Entry at index " << i << " should be transparent";
    }

    // Next 8 entries should be the original entries (tile indices 1-8)
    for (std::size_t i = 4; i < 12; ++i) {
        EXPECT_EQ(entries[i].tile_index(), i - 3) << "Entry at index " << i << " should have tile_index " << (i - 3);
    }
}

TEST_F(LayerModeConverterTests, TripleLayerizeCoveredLayerTypeInsertsTransparentAtEnd)
{
    // Create a dual-layer component with LayerType::covered
    auto component = create_dual_layer_component_single_metatile(LayerType::covered);

    auto result = converter_->triple_layerize(component);

    ASSERT_TRUE(result.has_value());
    const auto &entries = result.value();

    // Should have 12 entries total (8 original + 4 transparent)
    ASSERT_EQ(entries.size(), metatile::entries_per_metatile_triple);

    // First 8 entries should be the original entries (tile indices 1-8)
    for (std::size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(entries[i].tile_index(), i + 1) << "Entry at index " << i << " should have tile_index " << (i + 1);
    }

    // Last 4 entries should be transparent
    for (std::size_t i = 8; i < 12; ++i) {
        EXPECT_TRUE(entries[i].is_transparent()) << "Entry at index " << i << " should be transparent";
    }
}

TEST_F(LayerModeConverterTests, TripleLayerizeSplitLayerTypeInsertsTransparentInMiddle)
{
    // Create a dual-layer component with LayerType::split
    auto component = create_dual_layer_component_single_metatile(LayerType::split);

    auto result = converter_->triple_layerize(component);

    ASSERT_TRUE(result.has_value());
    const auto &entries = result.value();

    // Should have 12 entries total (4 original + 4 transparent + 4 original)
    ASSERT_EQ(entries.size(), metatile::entries_per_metatile_triple);

    // First 4 entries should be the first half of original entries (tile indices 1-4)
    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(entries[i].tile_index(), i + 1) << "Entry at index " << i << " should have tile_index " << (i + 1);
    }

    // Middle 4 entries should be transparent
    for (std::size_t i = 4; i < 8; ++i) {
        EXPECT_TRUE(entries[i].is_transparent()) << "Entry at index " << i << " should be transparent";
    }

    // Last 4 entries should be the second half of original entries (tile indices 5-8)
    for (std::size_t i = 8; i < 12; ++i) {
        EXPECT_EQ(entries[i].tile_index(), i - 3) << "Entry at index " << i << " should have tile_index " << (i - 3);
    }
}

TEST_F(LayerModeConverterTests, TripleLayerizeMultipleMetatilesWithDifferentLayerTypes)
{
    PorymapTilesetComponent component;

    // Add first metatile with LayerType::normal (tile indices 1-8)
    for (unsigned int i = 1; i <= 8; ++i) {
        component.push_back_tilemap_entry(create_test_entry(i));
    }
    component.push_back_attribute(MetatileAttribute{LayerType::normal, 0});

    // Add second metatile with LayerType::covered (tile indices 9-16)
    for (unsigned int i = 9; i <= 16; ++i) {
        component.push_back_tilemap_entry(create_test_entry(i));
    }
    component.push_back_attribute(MetatileAttribute{LayerType::covered, 0});

    // Add third metatile with LayerType::split (tile indices 17-24)
    for (unsigned int i = 17; i <= 24; ++i) {
        component.push_back_tilemap_entry(create_test_entry(i));
    }
    component.push_back_attribute(MetatileAttribute{LayerType::split, 0});

    auto result = converter_->triple_layerize(component);

    ASSERT_TRUE(result.has_value());
    const auto &entries = result.value();

    // Should have 36 entries total (3 metatiles * 12 entries each)
    ASSERT_EQ(entries.size(), 36);

    // Verify first metatile (normal: 4 transparent + 8 original)
    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_TRUE(entries[i].is_transparent());
    }
    for (std::size_t i = 4; i < 12; ++i) {
        EXPECT_EQ(entries[i].tile_index(), i - 3);
    }

    // Verify second metatile (covered: 8 original + 4 transparent)
    for (std::size_t i = 12; i < 20; ++i) {
        EXPECT_EQ(entries[i].tile_index(), i - 3);
    }
    for (std::size_t i = 20; i < 24; ++i) {
        EXPECT_TRUE(entries[i].is_transparent());
    }

    // Verify third metatile (split: 4 original + 4 transparent + 4 original)
    for (std::size_t i = 24; i < 28; ++i) {
        EXPECT_EQ(entries[i].tile_index(), i - 7);
    }
    for (std::size_t i = 28; i < 32; ++i) {
        EXPECT_TRUE(entries[i].is_transparent());
    }
    for (std::size_t i = 32; i < 36; ++i) {
        EXPECT_EQ(entries[i].tile_index(), i - 11);
    }
}

TEST_F(LayerModeConverterTests, TripleLayerizePreservesNonZeroPalIndex)
{
    PorymapTilesetComponent component;

    // Add entries with different palette indices
    for (unsigned int i = 1; i <= 8; ++i) {
        component.push_back_tilemap_entry(create_test_entry(i, i % 4)); // Use different pal indices
    }
    component.push_back_attribute(MetatileAttribute{LayerType::normal, 0});

    auto result = converter_->triple_layerize(component);

    ASSERT_TRUE(result.has_value());
    const auto &entries = result.value();

    // Verify that the original entries preserve their palette indices
    for (std::size_t i = 4; i < 12; ++i) {
        EXPECT_EQ(entries[i].pal_index(), (i - 3) % 4);
    }
}

TEST_F(LayerModeConverterTests, TripleLayerizePreservesFlipFlags)
{
    PorymapTilesetComponent component;

    // Add entries with flip flags
    for (unsigned int i = 1; i <= 8; ++i) {
        bool hflip = (i % 2 == 0);
        bool vflip = (i % 3 == 0);
        component.push_back_tilemap_entry(TilemapEntry{i, 0, hflip, vflip});
    }
    component.push_back_attribute(MetatileAttribute{LayerType::covered, 0});

    auto result = converter_->triple_layerize(component);

    ASSERT_TRUE(result.has_value());
    const auto &entries = result.value();

    // Verify that flip flags are preserved
    for (std::size_t i = 0; i < 8; ++i) {
        bool expected_hflip = ((i + 1) % 2 == 0);
        bool expected_vflip = ((i + 1) % 3 == 0);
        EXPECT_EQ(entries[i].h_flip(), expected_hflip) << "hflip mismatch at index " << i;
        EXPECT_EQ(entries[i].v_flip(), expected_vflip) << "vflip mismatch at index " << i;
    }
}

// ===== dual_layerize tests =====

TEST_F(LayerModeConverterTests, DualLayerizeNormalLayerTypeRemovesTransparentFromStart)
{
    // Create triple-layer entries (12 entries: 4 transparent + 8 non-transparent)
    std::vector<TilemapEntry> triple_entries;
    // First 4 transparent
    for (unsigned int i = 0; i < 4; ++i) {
        triple_entries.push_back(TilemapEntry{0, 0, false, false});
    }
    // Next 8 non-transparent (tile indices 1-8)
    for (unsigned int i = 1; i <= 8; ++i) {
        triple_entries.push_back(create_test_entry(i));
    }

    // Create source metatile with LayerType::normal
    std::vector<Metatile<Rgba32>> source_metatiles;
    source_metatiles.push_back(create_metatile_with_layer_type(LayerType::normal));

    auto dual_entries = converter_->dual_layerize(triple_entries, source_metatiles);

    // Should have 8 entries (removed first 4 transparent)
    ASSERT_EQ(dual_entries.size(), metatile::entries_per_metatile_dual);

    // Verify entries are tile indices 1-8
    for (std::size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(dual_entries[i].tile_index(), i + 1)
            << "Entry at index " << i << " should have tile_index " << (i + 1);
    }
}

TEST_F(LayerModeConverterTests, DualLayerizeCoveredLayerTypeRemovesTransparentFromEnd)
{
    // Create triple-layer entries (12 entries: 8 non-transparent + 4 transparent)
    std::vector<TilemapEntry> triple_entries;
    // First 8 non-transparent (tile indices 1-8)
    for (unsigned int i = 1; i <= 8; ++i) {
        triple_entries.push_back(create_test_entry(i));
    }
    // Last 4 transparent
    for (unsigned int i = 0; i < 4; ++i) {
        triple_entries.push_back(TilemapEntry{0, 0, false, false});
    }

    // Create source metatile with LayerType::covered
    std::vector<Metatile<Rgba32>> source_metatiles;
    source_metatiles.push_back(create_metatile_with_layer_type(LayerType::covered));

    auto dual_entries = converter_->dual_layerize(triple_entries, source_metatiles);

    // Should have 8 entries (removed last 4 transparent)
    ASSERT_EQ(dual_entries.size(), metatile::entries_per_metatile_dual);

    // Verify entries are tile indices 1-8
    for (std::size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(dual_entries[i].tile_index(), i + 1)
            << "Entry at index " << i << " should have tile_index " << (i + 1);
    }
}

TEST_F(LayerModeConverterTests, DualLayerizeSplitLayerTypeRemovesTransparentFromMiddle)
{
    // Create triple-layer entries (12 entries: 4 non-transparent + 4 transparent + 4 non-transparent)
    std::vector<TilemapEntry> triple_entries;
    // First 4 non-transparent (tile indices 1-4)
    for (unsigned int i = 1; i <= 4; ++i) {
        triple_entries.push_back(create_test_entry(i));
    }
    // Middle 4 transparent
    for (unsigned int i = 0; i < 4; ++i) {
        triple_entries.push_back(TilemapEntry{0, 0, false, false});
    }
    // Last 4 non-transparent (tile indices 5-8)
    for (unsigned int i = 5; i <= 8; ++i) {
        triple_entries.push_back(create_test_entry(i));
    }

    // Create source metatile with LayerType::split
    std::vector<Metatile<Rgba32>> source_metatiles;
    source_metatiles.push_back(create_metatile_with_layer_type(LayerType::split));

    auto dual_entries = converter_->dual_layerize(triple_entries, source_metatiles);

    // Should have 8 entries (removed middle 4 transparent)
    ASSERT_EQ(dual_entries.size(), metatile::entries_per_metatile_dual);

    // Verify entries are tile indices 1-8
    for (std::size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(dual_entries[i].tile_index(), i + 1)
            << "Entry at index " << i << " should have tile_index " << (i + 1);
    }
}

TEST_F(LayerModeConverterTests, DualLayerizeMultipleMetatilesWithDifferentLayerTypes)
{
    std::vector<TilemapEntry> triple_entries;

    // First metatile (normal): 4 transparent + 8 non-transparent (tile indices 1-8)
    for (unsigned int i = 0; i < 4; ++i) {
        triple_entries.push_back(TilemapEntry{0, 0, false, false});
    }
    for (unsigned int i = 1; i <= 8; ++i) {
        triple_entries.push_back(create_test_entry(i));
    }

    // Second metatile (covered): 8 non-transparent (tile indices 9-16) + 4 transparent
    for (unsigned int i = 9; i <= 16; ++i) {
        triple_entries.push_back(create_test_entry(i));
    }
    for (unsigned int i = 0; i < 4; ++i) {
        triple_entries.push_back(TilemapEntry{0, 0, false, false});
    }

    // Third metatile (split): 4 non-transparent (17-20) + 4 transparent + 4 non-transparent (21-24)
    for (unsigned int i = 17; i <= 20; ++i) {
        triple_entries.push_back(create_test_entry(i));
    }
    for (unsigned int i = 0; i < 4; ++i) {
        triple_entries.push_back(TilemapEntry{0, 0, false, false});
    }
    for (unsigned int i = 21; i <= 24; ++i) {
        triple_entries.push_back(create_test_entry(i));
    }

    // Create source metatiles
    std::vector<Metatile<Rgba32>> source_metatiles;
    source_metatiles.push_back(create_metatile_with_layer_type(LayerType::normal));
    source_metatiles.push_back(create_metatile_with_layer_type(LayerType::covered));
    source_metatiles.push_back(create_metatile_with_layer_type(LayerType::split));

    auto dual_entries = converter_->dual_layerize(triple_entries, source_metatiles);

    // Should have 24 entries total (3 metatiles * 8 entries each)
    ASSERT_EQ(dual_entries.size(), 24);

    // Verify all entries have correct tile indices (1-24)
    for (std::size_t i = 0; i < 24; ++i) {
        EXPECT_EQ(dual_entries[i].tile_index(), i + 1);
    }
}

// ===== Round-trip tests =====

TEST_F(LayerModeConverterTests, RoundTripNormalLayerType)
{
    // Start with dual-layer component
    auto dual_component = create_dual_layer_component_single_metatile(LayerType::normal);
    const auto &original_entries = dual_component.metatiles_bin();

    // Triple-layerize
    auto triple_result = converter_->triple_layerize(dual_component);
    ASSERT_TRUE(triple_result.has_value());
    const auto &triple_entries = triple_result.value();

    // Create source metatile for dual-layerize
    std::vector<Metatile<Rgba32>> source_metatiles;
    source_metatiles.push_back(create_metatile_with_layer_type(LayerType::normal));

    // Dual-layerize back
    auto final_entries = converter_->dual_layerize(triple_entries, source_metatiles);

    // Verify round-trip produces identical result
    ASSERT_EQ(final_entries.size(), original_entries.size());
    for (std::size_t i = 0; i < original_entries.size(); ++i) {
        EXPECT_EQ(final_entries[i].tile_index(), original_entries[i].tile_index()) << "Mismatch at index " << i;
        EXPECT_EQ(final_entries[i].pal_index(), original_entries[i].pal_index()) << "Mismatch at index " << i;
        EXPECT_EQ(final_entries[i].h_flip(), original_entries[i].h_flip()) << "Mismatch at index " << i;
        EXPECT_EQ(final_entries[i].v_flip(), original_entries[i].v_flip()) << "Mismatch at index " << i;
    }
}

TEST_F(LayerModeConverterTests, RoundTripCoveredLayerType)
{
    // Start with dual-layer component
    auto dual_component = create_dual_layer_component_single_metatile(LayerType::covered);
    const auto &original_entries = dual_component.metatiles_bin();

    // Triple-layerize
    auto triple_result = converter_->triple_layerize(dual_component);
    ASSERT_TRUE(triple_result.has_value());
    const auto &triple_entries = triple_result.value();

    // Create source metatile for dual-layerize
    std::vector<Metatile<Rgba32>> source_metatiles;
    source_metatiles.push_back(create_metatile_with_layer_type(LayerType::covered));

    // Dual-layerize back
    auto final_entries = converter_->dual_layerize(triple_entries, source_metatiles);

    // Verify round-trip produces identical result
    ASSERT_EQ(final_entries.size(), original_entries.size());
    for (std::size_t i = 0; i < original_entries.size(); ++i) {
        EXPECT_EQ(final_entries[i].tile_index(), original_entries[i].tile_index()) << "Mismatch at index " << i;
        EXPECT_EQ(final_entries[i].pal_index(), original_entries[i].pal_index()) << "Mismatch at index " << i;
        EXPECT_EQ(final_entries[i].h_flip(), original_entries[i].h_flip()) << "Mismatch at index " << i;
        EXPECT_EQ(final_entries[i].v_flip(), original_entries[i].v_flip()) << "Mismatch at index " << i;
    }
}

TEST_F(LayerModeConverterTests, RoundTripSplitLayerType)
{
    // Start with dual-layer component
    auto dual_component = create_dual_layer_component_single_metatile(LayerType::split);
    const auto &original_entries = dual_component.metatiles_bin();

    // Triple-layerize
    auto triple_result = converter_->triple_layerize(dual_component);
    ASSERT_TRUE(triple_result.has_value());
    const auto &triple_entries = triple_result.value();

    // Create source metatile for dual-layerize
    std::vector<Metatile<Rgba32>> source_metatiles;
    source_metatiles.push_back(create_metatile_with_layer_type(LayerType::split));

    // Dual-layerize back
    auto final_entries = converter_->dual_layerize(triple_entries, source_metatiles);

    // Verify round-trip produces identical result
    ASSERT_EQ(final_entries.size(), original_entries.size());
    for (std::size_t i = 0; i < original_entries.size(); ++i) {
        EXPECT_EQ(final_entries[i].tile_index(), original_entries[i].tile_index()) << "Mismatch at index " << i;
        EXPECT_EQ(final_entries[i].pal_index(), original_entries[i].pal_index()) << "Mismatch at index " << i;
        EXPECT_EQ(final_entries[i].h_flip(), original_entries[i].h_flip()) << "Mismatch at index " << i;
        EXPECT_EQ(final_entries[i].v_flip(), original_entries[i].v_flip()) << "Mismatch at index " << i;
    }
}

TEST_F(LayerModeConverterTests, RoundTripMultipleMetatiles)
{
    PorymapTilesetComponent dual_component;

    // Add first metatile with LayerType::normal (tile indices 1-8)
    for (unsigned int i = 1; i <= 8; ++i) {
        dual_component.push_back_tilemap_entry(create_test_entry(i));
    }
    dual_component.push_back_attribute(MetatileAttribute{LayerType::normal, 0});

    // Add second metatile with LayerType::covered (tile indices 9-16)
    for (unsigned int i = 9; i <= 16; ++i) {
        dual_component.push_back_tilemap_entry(create_test_entry(i));
    }
    dual_component.push_back_attribute(MetatileAttribute{LayerType::covered, 0});

    // Add third metatile with LayerType::split (tile indices 17-24)
    for (unsigned int i = 17; i <= 24; ++i) {
        dual_component.push_back_tilemap_entry(create_test_entry(i));
    }
    dual_component.push_back_attribute(MetatileAttribute{LayerType::split, 0});

    const auto &original_entries = dual_component.metatiles_bin();

    // Triple-layerize
    auto triple_result = converter_->triple_layerize(dual_component);
    ASSERT_TRUE(triple_result.has_value());
    const auto &triple_entries = triple_result.value();

    // Create source metatiles for dual-layerize
    std::vector<Metatile<Rgba32>> source_metatiles;
    source_metatiles.push_back(create_metatile_with_layer_type(LayerType::normal));
    source_metatiles.push_back(create_metatile_with_layer_type(LayerType::covered));
    source_metatiles.push_back(create_metatile_with_layer_type(LayerType::split));

    // Dual-layerize back
    auto final_entries = converter_->dual_layerize(triple_entries, source_metatiles);

    // Verify round-trip produces identical result
    ASSERT_EQ(final_entries.size(), original_entries.size());
    for (std::size_t i = 0; i < original_entries.size(); ++i) {
        EXPECT_EQ(final_entries[i].tile_index(), original_entries[i].tile_index()) << "Mismatch at index " << i;
        EXPECT_EQ(final_entries[i].pal_index(), original_entries[i].pal_index()) << "Mismatch at index " << i;
        EXPECT_EQ(final_entries[i].h_flip(), original_entries[i].h_flip()) << "Mismatch at index " << i;
        EXPECT_EQ(final_entries[i].v_flip(), original_entries[i].v_flip()) << "Mismatch at index " << i;
    }
}
