#include "gtest/gtest.h"

#include <filesystem>
#include <unordered_map>

#include "porytiles/domain/models/base_game.hpp"
#include "porytiles/domain/models/layer.hpp"
#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/services/enum_map_provider.hpp"
#include "porytiles/infra/services/attributes_csv_loader.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"

using namespace porytiles;

namespace {

const std::filesystem::path test_resources_dir = "resources/tests/integration/infra/services/attributes_csv";

// One stub serves every field now that providers are a single interface. The noun feeds its own error
// strings so the "unknown terrain type"/"unknown encounter type" expectations below still hold.
class StubEnumMapProvider final : public EnumMapProvider {
  public:
    StubEnumMapProvider(std::unordered_map<std::string, std::uint32_t> name_to_value, std::string noun)
        : name_to_value_{std::move(name_to_value)}, noun_{std::move(noun)}
    {
        for (const auto &[name, value] : name_to_value_) {
            value_to_name_[value] = name;
        }
    }

    [[nodiscard]] ChainableResult<std::uint32_t> lookup(const std::string &name) const override
    {
        auto it = name_to_value_.find(name);
        if (it == name_to_value_.end()) {
            return FormattableError{"unknown {}: {}", FormatParam{noun_}, FormatParam{name}};
        }
        return it->second;
    }

    [[nodiscard]] ChainableResult<std::string> lookup(std::uint32_t value) const override
    {
        auto it = value_to_name_.find(value);
        if (it == value_to_name_.end()) {
            return FormattableError{"unknown {} value: {}", FormatParam{noun_}, FormatParam{value}};
        }
        return it->second;
    }

  private:
    std::unordered_map<std::string, std::uint32_t> name_to_value_{};
    std::unordered_map<std::uint32_t, std::string> value_to_name_{};
    std::string noun_;
};

class AttributesCsvLoaderTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_{};
    StubEnumMapProvider behavior_map_{
        {{"MB_NORMAL", 0x00}, {"MB_TALL_GRASS", 0x02}, {"MB_DEEP_WATER", 0x12}, {"MB_COUNTER", 0x80}}, "behavior"};
    StubEnumMapProvider terrain_map_{
        {{"TILE_TERRAIN_NORMAL", 0}, {"TILE_TERRAIN_GRASS", 1}, {"TILE_TERRAIN_WATER", 2}}, "terrain type"};
    StubEnumMapProvider encounter_map_{
        {{"TILE_ENCOUNTER_NONE", 0}, {"TILE_ENCOUNTER_LAND", 1}, {"TILE_ENCOUNTER_WATER", 2}}, "encounter type"};
};

} // namespace

TEST_F(AttributesCsvLoaderTest, LoadValidCsvReturnsCorrectAttributes)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(test_resources_dir / "valid.csv");
    ASSERT_TRUE(result.has_value()) << "Expected successful load";

    const auto &attributes = result.value();
    ASSERT_EQ(attributes.size(), 3);

    // Check metatile 0: MB_NORMAL (0x00)
    ASSERT_TRUE(attributes.contains(0));
    EXPECT_EQ(attributes.at(0).field(attr::field_behavior), 0x00u);
    EXPECT_EQ(attributes.at(0).layer_type(), LayerType::normal);

    // Check metatile 1: MB_TALL_GRASS (0x02)
    ASSERT_TRUE(attributes.contains(1));
    EXPECT_EQ(attributes.at(1).field(attr::field_behavior), 0x02u);
    EXPECT_EQ(attributes.at(1).layer_type(), LayerType::normal);

    // Check metatile 2: MB_DEEP_WATER (0x12)
    ASSERT_TRUE(attributes.contains(2));
    EXPECT_EQ(attributes.at(2).field(attr::field_behavior), 0x12u);
    EXPECT_EQ(attributes.at(2).layer_type(), LayerType::normal);
}

TEST_F(AttributesCsvLoaderTest, LoadValidFireredCsvReturnsCorrectAttributes)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_, BaseGame::pokefirered, &terrain_map_, &encounter_map_};

    auto result = loader.load(test_resources_dir / "valid_firered.csv");
    ASSERT_TRUE(result.has_value()) << "Expected successful load";

    const auto &attributes = result.value();
    ASSERT_EQ(attributes.size(), 3);

    // Check metatile 0: MB_NORMAL, TILE_TERRAIN_NORMAL (0), TILE_ENCOUNTER_NONE (0)
    ASSERT_TRUE(attributes.contains(0));
    EXPECT_EQ(attributes.at(0).field(attr::field_behavior), 0x00u);
    EXPECT_EQ(attributes.at(0).field(attr::field_terrain), 0u);
    EXPECT_EQ(attributes.at(0).field(attr::field_encounter_type), 0u);
    EXPECT_EQ(attributes.at(0).layer_type(), LayerType::normal);

    // Check metatile 1: MB_TALL_GRASS, TILE_TERRAIN_GRASS (1), TILE_ENCOUNTER_LAND (1)
    ASSERT_TRUE(attributes.contains(1));
    EXPECT_EQ(attributes.at(1).field(attr::field_behavior), 0x02u);
    EXPECT_EQ(attributes.at(1).field(attr::field_terrain), 1u);
    EXPECT_EQ(attributes.at(1).field(attr::field_encounter_type), 1u);

    // Check metatile 2: MB_DEEP_WATER, TILE_TERRAIN_WATER (2), TILE_ENCOUNTER_WATER (2)
    ASSERT_TRUE(attributes.contains(2));
    EXPECT_EQ(attributes.at(2).field(attr::field_behavior), 0x12u);
    EXPECT_EQ(attributes.at(2).field(attr::field_terrain), 2u);
    EXPECT_EQ(attributes.at(2).field(attr::field_encounter_type), 2u);
}

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvWithoutBaseGameAutoDetects)
{
    // No base_game provided, but terrain/encounter providers are available
    AttributesCsvLoader loader{&formatter_, &behavior_map_, std::nullopt, &terrain_map_, &encounter_map_};

    auto result = loader.load(test_resources_dir / "valid_firered.csv");
    ASSERT_TRUE(result.has_value()) << "Expected auto-detection to succeed";

    const auto &attributes = result.value();
    ASSERT_EQ(attributes.size(), 3);

    // Verify terrain/encounter values were parsed correctly
    EXPECT_EQ(attributes.at(1).field(attr::field_terrain), 1u);
    EXPECT_EQ(attributes.at(1).field(attr::field_encounter_type), 1u);
}

TEST_F(AttributesCsvLoaderTest, LoadNonExistentFileReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(test_resources_dir / "does_not_exist.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("does not exist") != std::string::npos)
        << "Error should mention file does not exist. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadEmptyFileReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(test_resources_dir / "empty.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("empty") != std::string::npos)
        << "Error should mention file is empty. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadInvalidHeaderSingleColumnReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(test_resources_dir / "invalid_header_single_column.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("invalid header") != std::string::npos)
        << "Error should mention invalid header. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadInvalidHeaderWrongNamesReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(test_resources_dir / "invalid_header_wrong_names.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("invalid header") != std::string::npos)
        << "Error should mention invalid header. Got: " << error_text;
    EXPECT_TRUE(error_text.find("id,behavior") != std::string::npos)
        << "Error should show expected format. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadInvalidIdNotIntegerReturnsErrorWithContext)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(test_resources_dir / "invalid_id_not_integer.csv");
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

    auto result = loader.load(test_resources_dir / "negative_id.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("cannot be negative") != std::string::npos)
        << "Error should mention negative id. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadMissingColumnsReturnsErrorWithContext)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(test_resources_dir / "missing_columns.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("expected at least 2 columns") != std::string::npos)
        << "Error should mention expected columns. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadDuplicateIdReturnsErrorWithBothLocations)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(test_resources_dir / "duplicate_id.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);

    EXPECT_TRUE(error_text.find("duplicate metatile id") != std::string::npos)
        << "Error should mention duplicate metatile id. Got: " << error_text;

    EXPECT_TRUE(error_text.find("1") != std::string::npos)
        << "Error should show the duplicate id '1'. Got: " << error_text;

    EXPECT_TRUE(error_text.find("note:") != std::string::npos)
        << "Error should contain 'note:' for original location. Got: " << error_text;

    EXPECT_TRUE(error_text.find("originally defined") != std::string::npos)
        << "Error should mention originally defined. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadUnknownBehaviorReturnsErrorWithContext)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(test_resources_dir / "unknown_behavior.csv");
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

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvWithEmeraldBaseGameReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_, BaseGame::pokeemerald, &terrain_map_, &encounter_map_};

    auto result = loader.load(test_resources_dir / "valid_firered.csv");
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

    auto result = loader.load(test_resources_dir / "valid.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("Emerald format") != std::string::npos)
        << "Error should mention Emerald format. Got: " << error_text;
    EXPECT_TRUE(error_text.find("pokefirered") != std::string::npos)
        << "Error should mention base game. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvWithoutTerrainProviderPanics)
{
    // FireRed CSV but no terrain provider -- should panic
    AttributesCsvLoader loader{&formatter_, &behavior_map_, std::nullopt, nullptr, &encounter_map_};

    EXPECT_DEATH(std::ignore = loader.load(test_resources_dir / "valid_firered.csv"), "terrain type provider");
}

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvWithoutEncounterProviderPanics)
{
    // FireRed CSV but no encounter provider -- should panic
    AttributesCsvLoader loader{&formatter_, &behavior_map_, std::nullopt, &terrain_map_, nullptr};

    EXPECT_DEATH(std::ignore = loader.load(test_resources_dir / "valid_firered.csv"), "encounter type provider");
}

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvUnknownTerrainTypeReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_, BaseGame::pokefirered, &terrain_map_, &encounter_map_};

    auto result = loader.load(test_resources_dir / "firered_unknown_terrain.csv");
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

    auto result = loader.load(test_resources_dir / "firered_unknown_encounter.csv");
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

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvRowTooFewColumnsReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_, BaseGame::pokefirered, &terrain_map_, &encounter_map_};

    auto result = loader.load(test_resources_dir / "firered_row_too_few_columns.csv");
    EXPECT_FALSE(result.has_value());

    std::string full_error_text{};
    for (const auto &err : result.chain()) {
        full_error_text += err->join(formatter_);
        full_error_text += "\n";
    }

    EXPECT_TRUE(full_error_text.find("expected 4 columns") != std::string::npos)
        << "Error should mention expected 4 columns. Got: " << full_error_text;
}
