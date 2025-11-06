#include "gtest/gtest.h"

#include "porytiles2/domain/models/metatile.hpp"
#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/models/porymap_tileset_component.hpp"
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
PorymapTilesetComponent create_dual_layer_component_single_metatile(attr::LayerType layer_type)
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
PorymapTilesetComponent create_triple_layer_component_single_metatile(attr::LayerType layer_type)
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

} // namespace

class LayerModeConverterTests : public ::testing::Test {
  protected:
    void SetUp() override
    {
        format_ = std::make_unique<PlainTextFormatter>();
        diag_ = std::make_unique<StderrStyledUserDiagnostics>(format_.get());
        tile_printer_ = std::make_unique<AsciiTilePrinter>(format_.get());
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
    auto component = create_triple_layer_component_single_metatile(attr::LayerType::normal);

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
    auto component = create_dual_layer_component_single_metatile(attr::LayerType::normal);

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
    auto component = create_dual_layer_component_single_metatile(attr::LayerType::covered);

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
    auto component = create_dual_layer_component_single_metatile(attr::LayerType::split);

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
    component.push_back_attribute(MetatileAttribute{attr::LayerType::normal, 0});

    // Add second metatile with LayerType::covered (tile indices 9-16)
    for (unsigned int i = 9; i <= 16; ++i) {
        component.push_back_tilemap_entry(create_test_entry(i));
    }
    component.push_back_attribute(MetatileAttribute{attr::LayerType::covered, 0});

    // Add third metatile with LayerType::split (tile indices 17-24)
    for (unsigned int i = 17; i <= 24; ++i) {
        component.push_back_tilemap_entry(create_test_entry(i));
    }
    component.push_back_attribute(MetatileAttribute{attr::LayerType::split, 0});

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
    component.push_back_attribute(MetatileAttribute{attr::LayerType::normal, 0});

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
    component.push_back_attribute(MetatileAttribute{attr::LayerType::covered, 0});

    auto result = converter_->triple_layerize(component);

    ASSERT_TRUE(result.has_value());
    const auto &entries = result.value();

    // Verify that flip flags are preserved
    for (std::size_t i = 0; i < 8; ++i) {
        bool expected_hflip = ((i + 1) % 2 == 0);
        bool expected_vflip = ((i + 1) % 3 == 0);
        EXPECT_EQ(entries[i].hflip(), expected_hflip) << "hflip mismatch at index " << i;
        EXPECT_EQ(entries[i].vflip(), expected_vflip) << "vflip mismatch at index " << i;
    }
}
