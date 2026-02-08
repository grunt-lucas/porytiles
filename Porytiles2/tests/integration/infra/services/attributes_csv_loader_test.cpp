#include "gtest/gtest.h"

#include <filesystem>
#include <unordered_map>

#include "porytiles2/domain/models/base_game.hpp"
#include "porytiles2/domain/models/layer.hpp"
#include "porytiles2/domain/services/behavior_map_provider.hpp"
#include "porytiles2/domain/services/encounter_type_map_provider.hpp"
#include "porytiles2/domain/services/terrain_type_map_provider.hpp"
#include "porytiles2/infra/services/attributes_csv_loader.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"

using namespace porytiles2;

namespace {

const std::filesystem::path kTestResourcesDir = "Resources/Tests/integration/infra/services/attributes_csv";

/**
 * @brief A stub BehaviorMapProvider for testing that returns known values for known behavior names.
 */
class StubBehaviorMapProvider final : public BehaviorMapProvider {
  public:
    StubBehaviorMapProvider()
    {
        name_to_value_["MB_NORMAL"] = 0x00;
        name_to_value_["MB_TALL_GRASS"] = 0x02;
        name_to_value_["MB_DEEP_WATER"] = 0x12;
        name_to_value_["MB_COUNTER"] = 0x80;

        for (const auto &[name, value] : name_to_value_) {
            value_to_name_[value] = name;
        }
    }

    [[nodiscard]] ChainableResult<std::uint16_t> lookup(const std::string &behavior_name) const override
    {
        auto it = name_to_value_.find(behavior_name);
        if (it == name_to_value_.end()) {
            return FormattableError{"unknown behavior: {}", FormatParam{behavior_name}};
        }
        return it->second;
    }

    [[nodiscard]] ChainableResult<std::string> lookup(std::uint16_t behavior_value) const override
    {
        auto it = value_to_name_.find(behavior_value);
        if (it == value_to_name_.end()) {
            return FormattableError{"unknown behavior value: {}", FormatParam{behavior_value}};
        }
        return it->second;
    }

  private:
    std::unordered_map<std::string, std::uint16_t> name_to_value_{};
    std::unordered_map<std::uint16_t, std::string> value_to_name_{};
};

/**
 * @brief A stub TerrainTypeMapProvider for testing that returns known values for known terrain names.
 */
class StubTerrainTypeMapProvider final : public TerrainTypeMapProvider {
  public:
    StubTerrainTypeMapProvider()
    {
        name_to_value_["TILE_TERRAIN_NORMAL"] = 0;
        name_to_value_["TILE_TERRAIN_GRASS"] = 1;
        name_to_value_["TILE_TERRAIN_WATER"] = 2;

        for (const auto &[name, value] : name_to_value_) {
            value_to_name_[value] = name;
        }
    }

    [[nodiscard]] ChainableResult<std::uint8_t> lookup(const std::string &terrain_name) const override
    {
        auto it = name_to_value_.find(terrain_name);
        if (it == name_to_value_.end()) {
            return FormattableError{"unknown terrain type: {}", FormatParam{terrain_name}};
        }
        return it->second;
    }

    [[nodiscard]] ChainableResult<std::string> lookup(std::uint8_t terrain_value) const override
    {
        auto it = value_to_name_.find(terrain_value);
        if (it == value_to_name_.end()) {
            return FormattableError{"unknown terrain type value: {}", FormatParam{terrain_value}};
        }
        return it->second;
    }

  private:
    std::unordered_map<std::string, std::uint8_t> name_to_value_{};
    std::unordered_map<std::uint8_t, std::string> value_to_name_{};
};

/**
 * @brief A stub EncounterTypeMapProvider for testing that returns known values for known encounter names.
 */
class StubEncounterTypeMapProvider final : public EncounterTypeMapProvider {
  public:
    StubEncounterTypeMapProvider()
    {
        name_to_value_["TILE_ENCOUNTER_NONE"] = 0;
        name_to_value_["TILE_ENCOUNTER_LAND"] = 1;
        name_to_value_["TILE_ENCOUNTER_WATER"] = 2;

        for (const auto &[name, value] : name_to_value_) {
            value_to_name_[value] = name;
        }
    }

    [[nodiscard]] ChainableResult<std::uint8_t> lookup(const std::string &encounter_name) const override
    {
        auto it = name_to_value_.find(encounter_name);
        if (it == name_to_value_.end()) {
            return FormattableError{"unknown encounter type: {}", FormatParam{encounter_name}};
        }
        return it->second;
    }

    [[nodiscard]] ChainableResult<std::string> lookup(std::uint8_t encounter_value) const override
    {
        auto it = value_to_name_.find(encounter_value);
        if (it == value_to_name_.end()) {
            return FormattableError{"unknown encounter type value: {}", FormatParam{encounter_value}};
        }
        return it->second;
    }

  private:
    std::unordered_map<std::string, std::uint8_t> name_to_value_{};
    std::unordered_map<std::uint8_t, std::string> value_to_name_{};
};

class AttributesCsvLoaderTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_{};
    StubBehaviorMapProvider behavior_map_{};
    StubTerrainTypeMapProvider terrain_map_{};
    StubEncounterTypeMapProvider encounter_map_{};
};

} // namespace

// =============================================================================
// Valid CSV Tests (Emerald format)
// =============================================================================

TEST_F(AttributesCsvLoaderTest, LoadValidCsvReturnsCorrectAttributes)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(kTestResourcesDir / "valid.csv");
    ASSERT_TRUE(result.has_value()) << "Expected successful load";

    const auto &attributes = result.value();
    ASSERT_EQ(attributes.size(), 3);

    // Check metatile 0: MB_NORMAL (0x00)
    ASSERT_TRUE(attributes.contains(0));
    EXPECT_EQ(attributes.at(0).behavior(), 0x00);
    EXPECT_EQ(attributes.at(0).layer_type(), LayerType::normal);

    // Check metatile 1: MB_TALL_GRASS (0x02)
    ASSERT_TRUE(attributes.contains(1));
    EXPECT_EQ(attributes.at(1).behavior(), 0x02);
    EXPECT_EQ(attributes.at(1).layer_type(), LayerType::normal);

    // Check metatile 2: MB_DEEP_WATER (0x12)
    ASSERT_TRUE(attributes.contains(2));
    EXPECT_EQ(attributes.at(2).behavior(), 0x12);
    EXPECT_EQ(attributes.at(2).layer_type(), LayerType::normal);
}

// =============================================================================
// Valid CSV Tests (FireRed format)
// =============================================================================

TEST_F(AttributesCsvLoaderTest, LoadValidFireredCsvReturnsCorrectAttributes)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_, BaseGame::pokefirered, &terrain_map_, &encounter_map_};

    auto result = loader.load(kTestResourcesDir / "valid_firered.csv");
    ASSERT_TRUE(result.has_value()) << "Expected successful load";

    const auto &attributes = result.value();
    ASSERT_EQ(attributes.size(), 3);

    // Check metatile 0: MB_NORMAL, TILE_TERRAIN_NORMAL (0), TILE_ENCOUNTER_NONE (0)
    ASSERT_TRUE(attributes.contains(0));
    EXPECT_EQ(attributes.at(0).behavior(), 0x00);
    EXPECT_EQ(attributes.at(0).terrain(), 0);
    EXPECT_EQ(attributes.at(0).encounter_type(), 0);
    EXPECT_EQ(attributes.at(0).layer_type(), LayerType::normal);

    // Check metatile 1: MB_TALL_GRASS, TILE_TERRAIN_GRASS (1), TILE_ENCOUNTER_LAND (1)
    ASSERT_TRUE(attributes.contains(1));
    EXPECT_EQ(attributes.at(1).behavior(), 0x02);
    EXPECT_EQ(attributes.at(1).terrain(), 1);
    EXPECT_EQ(attributes.at(1).encounter_type(), 1);

    // Check metatile 2: MB_DEEP_WATER, TILE_TERRAIN_WATER (2), TILE_ENCOUNTER_WATER (2)
    ASSERT_TRUE(attributes.contains(2));
    EXPECT_EQ(attributes.at(2).behavior(), 0x12);
    EXPECT_EQ(attributes.at(2).terrain(), 2);
    EXPECT_EQ(attributes.at(2).encounter_type(), 2);
}

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvWithoutBaseGameAutoDetects)
{
    // No base_game provided, but terrain/encounter providers are available
    AttributesCsvLoader loader{&formatter_, &behavior_map_, std::nullopt, &terrain_map_, &encounter_map_};

    auto result = loader.load(kTestResourcesDir / "valid_firered.csv");
    ASSERT_TRUE(result.has_value()) << "Expected auto-detection to succeed";

    const auto &attributes = result.value();
    ASSERT_EQ(attributes.size(), 3);

    // Verify terrain/encounter values were parsed correctly
    EXPECT_EQ(attributes.at(1).terrain(), 1);
    EXPECT_EQ(attributes.at(1).encounter_type(), 1);
}

// =============================================================================
// File Error Tests
// =============================================================================

TEST_F(AttributesCsvLoaderTest, LoadNonExistentFileReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(kTestResourcesDir / "does_not_exist.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("does not exist") != std::string::npos)
        << "Error should mention file does not exist. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadEmptyFileReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(kTestResourcesDir / "empty.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("empty") != std::string::npos)
        << "Error should mention file is empty. Got: " << error_text;
}

// =============================================================================
// Header Validation Tests
// =============================================================================

TEST_F(AttributesCsvLoaderTest, LoadInvalidHeaderSingleColumnReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(kTestResourcesDir / "invalid_header_single_column.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("invalid header") != std::string::npos)
        << "Error should mention invalid header. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadInvalidHeaderWrongNamesReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(kTestResourcesDir / "invalid_header_wrong_names.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("invalid header") != std::string::npos)
        << "Error should mention invalid header. Got: " << error_text;
    EXPECT_TRUE(error_text.find("id,behavior") != std::string::npos)
        << "Error should show expected format. Got: " << error_text;
}

// =============================================================================
// Row Validation Tests
// =============================================================================

TEST_F(AttributesCsvLoaderTest, LoadInvalidIdNotIntegerReturnsErrorWithContext)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(kTestResourcesDir / "invalid_id_not_integer.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("invalid metatile id") != std::string::npos)
        << "Error should mention invalid metatile id. Got: " << error_text;
    EXPECT_TRUE(error_text.find("abc") != std::string::npos)
        << "Error should show the invalid value 'abc'. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadNegativeIdReturnsErrorWithContext)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(kTestResourcesDir / "negative_id.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("cannot be negative") != std::string::npos)
        << "Error should mention negative id. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadMissingColumnsReturnsErrorWithContext)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(kTestResourcesDir / "missing_columns.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("expected at least 2 columns") != std::string::npos)
        << "Error should mention expected columns. Got: " << error_text;
}

// =============================================================================
// Duplicate Detection Tests
// =============================================================================

TEST_F(AttributesCsvLoaderTest, LoadDuplicateIdReturnsErrorWithBothLocations)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(kTestResourcesDir / "duplicate_id.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);

    // Should mention duplicate
    EXPECT_TRUE(error_text.find("duplicate metatile id") != std::string::npos)
        << "Error should mention duplicate metatile id. Got: " << error_text;

    // Should mention the duplicate id value (1)
    EXPECT_TRUE(error_text.find("1") != std::string::npos)
        << "Error should show the duplicate id '1'. Got: " << error_text;

    // Should contain "note:" for original location
    EXPECT_TRUE(error_text.find("note:") != std::string::npos)
        << "Error should contain 'note:' for original location. Got: " << error_text;

    // Should mention "originally defined"
    EXPECT_TRUE(error_text.find("originally defined") != std::string::npos)
        << "Error should mention originally defined. Got: " << error_text;
}

// =============================================================================
// Behavior Lookup Tests
// =============================================================================

TEST_F(AttributesCsvLoaderTest, LoadUnknownBehaviorReturnsErrorWithContext)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(kTestResourcesDir / "unknown_behavior.csv");
    EXPECT_FALSE(result.has_value());

    // The error chain contains multiple errors - join all of them for the full message
    std::string full_error_text{};
    for (const auto &err : result.chain()) {
        full_error_text += err->join(formatter_);
        full_error_text += "\n";
    }

    EXPECT_TRUE(full_error_text.find("unknown metatile behavior") != std::string::npos)
        << "Error should mention unknown behavior. Got: " << full_error_text;
    EXPECT_TRUE(full_error_text.find("MB_DOES_NOT_EXIST") != std::string::npos)
        << "Error should show the unknown behavior name. Got: " << full_error_text;
}

// =============================================================================
// FireRed Format / BaseGame Mismatch Tests
// =============================================================================

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvWithEmeraldBaseGameReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_, BaseGame::pokeemerald, &terrain_map_, &encounter_map_};

    auto result = loader.load(kTestResourcesDir / "valid_firered.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("FireRed format") != std::string::npos)
        << "Error should mention FireRed format. Got: " << error_text;
    EXPECT_TRUE(error_text.find("pokeemerald") != std::string::npos)
        << "Error should mention base game. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadEmeraldCsvWithFireredBaseGameReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_, BaseGame::pokefirered, &terrain_map_, &encounter_map_};

    auto result = loader.load(kTestResourcesDir / "valid.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("Emerald format") != std::string::npos)
        << "Error should mention Emerald format. Got: " << error_text;
    EXPECT_TRUE(error_text.find("pokefirered") != std::string::npos)
        << "Error should mention base game. Got: " << error_text;
}

// =============================================================================
// FireRed Provider Availability Tests
// =============================================================================

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvWithoutTerrainProviderReturnsError)
{
    // FireRed CSV but no terrain provider
    AttributesCsvLoader loader{&formatter_, &behavior_map_, std::nullopt, nullptr, &encounter_map_};

    auto result = loader.load(kTestResourcesDir / "valid_firered.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("terrain type provider") != std::string::npos)
        << "Error should mention missing terrain provider. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvWithoutEncounterProviderReturnsError)
{
    // FireRed CSV but no encounter provider
    AttributesCsvLoader loader{&formatter_, &behavior_map_, std::nullopt, &terrain_map_, nullptr};

    auto result = loader.load(kTestResourcesDir / "valid_firered.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("encounter type provider") != std::string::npos)
        << "Error should mention missing encounter provider. Got: " << error_text;
}

// =============================================================================
// FireRed Lookup Error Tests
// =============================================================================

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvUnknownTerrainTypeReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_, BaseGame::pokefirered, &terrain_map_, &encounter_map_};

    auto result = loader.load(kTestResourcesDir / "firered_unknown_terrain.csv");
    EXPECT_FALSE(result.has_value());

    std::string full_error_text{};
    for (const auto &err : result.chain()) {
        full_error_text += err->join(formatter_);
        full_error_text += "\n";
    }

    EXPECT_TRUE(full_error_text.find("unknown terrain type") != std::string::npos)
        << "Error should mention unknown terrain type. Got: " << full_error_text;
    EXPECT_TRUE(full_error_text.find("TILE_TERRAIN_DOES_NOT_EXIST") != std::string::npos)
        << "Error should show the unknown terrain name. Got: " << full_error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvUnknownEncounterTypeReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_, BaseGame::pokefirered, &terrain_map_, &encounter_map_};

    auto result = loader.load(kTestResourcesDir / "firered_unknown_encounter.csv");
    EXPECT_FALSE(result.has_value());

    std::string full_error_text{};
    for (const auto &err : result.chain()) {
        full_error_text += err->join(formatter_);
        full_error_text += "\n";
    }

    EXPECT_TRUE(full_error_text.find("unknown encounter type") != std::string::npos)
        << "Error should mention unknown encounter type. Got: " << full_error_text;
    EXPECT_TRUE(full_error_text.find("TILE_ENCOUNTER_DOES_NOT_EXIST") != std::string::npos)
        << "Error should show the unknown encounter name. Got: " << full_error_text;
}

// =============================================================================
// FireRed Row Validation Tests
// =============================================================================

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvRowTooFewColumnsReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_, BaseGame::pokefirered, &terrain_map_, &encounter_map_};

    auto result = loader.load(kTestResourcesDir / "firered_row_too_few_columns.csv");
    EXPECT_FALSE(result.has_value());

    std::string full_error_text{};
    for (const auto &err : result.chain()) {
        full_error_text += err->join(formatter_);
        full_error_text += "\n";
    }

    EXPECT_TRUE(full_error_text.find("expected 4 columns") != std::string::npos)
        << "Error should mention expected 4 columns. Got: " << full_error_text;
}
