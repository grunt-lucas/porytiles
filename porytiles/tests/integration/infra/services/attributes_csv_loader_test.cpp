#include "gtest/gtest.h"

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <utility>

#include "porytiles/domain/models/layer.hpp"
#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/domain/services/enum_map_provider.hpp"
#include "porytiles/infra/services/attributes_csv_loader.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"
#include "support/mock_infra_config.hpp"

using namespace porytiles;

namespace {

const std::filesystem::path test_resources_dir = "resources/tests/integration/infra/services/attributes_csv";

// One stub serves every field now that providers are a single interface. The noun feeds its own error
// strings so the chained lookup errors below still identify their field.
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

// The ProviderSpec contents are irrelevant here (the tests stub the ProviderMap directly); the spec's presence is what
// marks a field provider-backed.
ProviderSpec dummy_provider_spec()
{
    return ProviderSpec{.header = "include/dummy.h", .prefix = "DUMMY_"};
}

// The stock emerald shape: a single provider-backed behavior field in a 2-byte attribute.
Schema make_emerald_schema()
{
    auto result = Schema::create({Field{"behavior", 0x00FF, 0, dummy_provider_spec()}}, 2);
    return std::move(result).value();
}

// The stock firered shape: seven fields in a 4-byte attribute, three provider-backed and four raw. Masks match the
// FRLG attribute bit layout from fieldmap.c (layer_type is structural and never a schema field).
Schema make_firered_schema()
{
    auto result = Schema::create(
        {
            Field{"behavior", 0x000001FF, 0, dummy_provider_spec()},
            Field{"terrain", 0x00003E00, 0, dummy_provider_spec()},
            Field{"attribute_2", 0x0003C000},
            Field{"attribute_3", 0x00FC0000},
            Field{"encounter_type", 0x07000000, 0, dummy_provider_spec()},
            Field{"attribute_5", 0x18000000},
            Field{"attribute_7", 0x80000000},
        },
        4);
    return std::move(result).value();
}

std::unique_ptr<StubEnumMapProvider> make_behavior_stub()
{
    return std::make_unique<StubEnumMapProvider>(
        std::unordered_map<std::string, std::uint32_t>{
            {"MB_NORMAL", 0x00}, {"MB_TALL_GRASS", 0x02}, {"MB_DEEP_WATER", 0x12}, {"MB_COUNTER", 0x80}},
        "behavior");
}

ProviderMap make_emerald_provider_map()
{
    ProviderMap providers{};
    providers.emplace("behavior", make_behavior_stub());
    return providers;
}

ProviderMap make_firered_provider_map()
{
    ProviderMap providers{};
    providers.emplace("behavior", make_behavior_stub());
    providers.emplace(
        "terrain",
        std::make_unique<StubEnumMapProvider>(
            std::unordered_map<std::string, std::uint32_t>{
                {"TILE_TERRAIN_NORMAL", 0}, {"TILE_TERRAIN_GRASS", 1}, {"TILE_TERRAIN_WATER", 2}},
            "terrain type"));
    providers.emplace(
        "encounter_type",
        std::make_unique<StubEnumMapProvider>(
            std::unordered_map<std::string, std::uint32_t>{
                {"TILE_ENCOUNTER_NONE", 0}, {"TILE_ENCOUNTER_LAND", 1}, {"TILE_ENCOUNTER_WATER", 2}},
            "encounter type"));
    return providers;
}

class AttributesCsvLoaderTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_{};
    Schema emerald_schema_ = make_emerald_schema();
    ProviderMap emerald_providers_ = make_emerald_provider_map();
    Schema firered_schema_ = make_firered_schema();
    ProviderMap firered_providers_ = make_firered_provider_map();
    MockInfraConfig config_{};
    BufferedUserDiagnostics diag_{};

    [[nodiscard]] AttributesCsvLoader emerald_loader() const
    {
        return AttributesCsvLoader{&formatter_, &emerald_schema_, &emerald_providers_, &config_, &diag_};
    }

    [[nodiscard]] AttributesCsvLoader firered_loader() const
    {
        return AttributesCsvLoader{&formatter_, &firered_schema_, &firered_providers_, &config_, &diag_};
    }

    [[nodiscard]] std::string join_error_chain(const auto &result) const
    {
        std::string full_error_text{};
        for (const auto &err : result.chain()) {
            full_error_text += err->join(formatter_);
            full_error_text += "\n";
        }
        return full_error_text;
    }
};

// The tileset scope passed to load(); MockInfraConfig ignores the scope and returns its member values.
constexpr auto kTilesetScope = "gTileset_Test";

} // namespace

TEST_F(AttributesCsvLoaderTest, LoadValidCsvReturnsCorrectAttributes)
{
    AttributesCsvLoader loader = emerald_loader();

    auto result = loader.load(test_resources_dir / "valid.csv", kTilesetScope);
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
    AttributesCsvLoader loader = firered_loader();

    auto result = loader.load(test_resources_dir / "valid_firered.csv", kTilesetScope);
    ASSERT_TRUE(result.has_value()) << "Expected successful load";

    const auto &attributes = result.value();
    ASSERT_EQ(attributes.size(), 3);

    // Check metatile 0: MB_NORMAL, TILE_TERRAIN_NORMAL (0), TILE_ENCOUNTER_NONE (0), raw fields 0
    ASSERT_TRUE(attributes.contains(0));
    EXPECT_EQ(attributes.at(0).field(attr::field_behavior), 0x00u);
    EXPECT_EQ(attributes.at(0).field(attr::field_terrain), 0u);
    EXPECT_EQ(attributes.at(0).field(attr::field_encounter_type), 0u);
    EXPECT_EQ(attributes.at(0).field(attr::field_attribute_2), 0u);
    EXPECT_EQ(attributes.at(0).field(attr::field_attribute_7), 0u);
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

TEST_F(AttributesCsvLoaderTest, LoadNonExistentFileReturnsError)
{
    AttributesCsvLoader loader = emerald_loader();

    auto result = loader.load(test_resources_dir / "does_not_exist.csv", kTilesetScope);
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("does not exist") != std::string::npos)
        << "Error should mention file does not exist. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadEmptyFileReturnsError)
{
    AttributesCsvLoader loader = emerald_loader();

    auto result = loader.load(test_resources_dir / "empty.csv", kTilesetScope);
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("empty") != std::string::npos)
        << "Error should mention file is empty. Got: " << error_text;
    EXPECT_TRUE(error_text.find("id,behavior") != std::string::npos)
        << "Error should show the schema's expected header. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadInvalidHeaderSingleColumnReturnsError)
{
    AttributesCsvLoader loader = emerald_loader();

    auto result = loader.load(test_resources_dir / "invalid_header_single_column.csv", kTilesetScope);
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("invalid header") != std::string::npos)
        << "Error should mention invalid header. Got: " << error_text;
    EXPECT_TRUE(error_text.find("missing column 'behavior'") != std::string::npos)
        << "Error should name the missing column. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadInvalidHeaderWrongNamesReturnsError)
{
    AttributesCsvLoader loader = emerald_loader();

    auto result = loader.load(test_resources_dir / "invalid_header_wrong_names.csv", kTilesetScope);
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("invalid header") != std::string::npos)
        << "Error should mention invalid header. Got: " << error_text;
    EXPECT_TRUE(error_text.find("id,behavior") != std::string::npos)
        << "Error should show the schema's expected header. Got: " << error_text;
}

// A CSV written for a wider schema fails the header cross-check instead of silently dropping its extra field columns.
// Together with the missing-column direction below, this replaces the old base-game format cross-check.
TEST_F(AttributesCsvLoaderTest, LoadFireredCsvWithEmeraldSchemaReturnsError)
{
    AttributesCsvLoader loader = emerald_loader();

    auto result = loader.load(test_resources_dir / "valid_firered.csv", kTilesetScope);
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("unexpected column 'terrain'") != std::string::npos)
        << "Error should name the unexpected column. Got: " << error_text;
    EXPECT_TRUE(error_text.find("id,behavior") != std::string::npos)
        << "Error should show the schema's expected header. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadEmeraldCsvWithFireredSchemaReturnsError)
{
    AttributesCsvLoader loader = firered_loader();

    auto result = loader.load(test_resources_dir / "valid.csv", kTilesetScope);
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("missing column 'terrain' at position 3") != std::string::npos)
        << "Error should name the missing column and its position. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadInvalidIdNotIntegerReturnsErrorWithContext)
{
    AttributesCsvLoader loader = emerald_loader();

    auto result = loader.load(test_resources_dir / "invalid_id_not_integer.csv", kTilesetScope);
    EXPECT_FALSE(result.has_value());

    std::string error_text = join_error_chain(result);
    EXPECT_TRUE(error_text.find("invalid metatile id") != std::string::npos)
        << "Error should mention invalid metatile id. Got: " << error_text;
    EXPECT_TRUE(error_text.find("abc") != std::string::npos)
        << "Error should show the invalid value 'abc'. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadNegativeIdReturnsErrorWithContext)
{
    AttributesCsvLoader loader = emerald_loader();

    auto result = loader.load(test_resources_dir / "negative_id.csv", kTilesetScope);
    EXPECT_FALSE(result.has_value());

    std::string error_text = join_error_chain(result);
    EXPECT_TRUE(error_text.find("cannot be negative") != std::string::npos)
        << "Error should mention negative id. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadMissingColumnsReturnsErrorWithContext)
{
    AttributesCsvLoader loader = emerald_loader();

    auto result = loader.load(test_resources_dir / "missing_columns.csv", kTilesetScope);
    EXPECT_FALSE(result.has_value());

    std::string error_text = join_error_chain(result);
    EXPECT_TRUE(error_text.find("expected at least 2 columns") != std::string::npos)
        << "Error should mention expected columns. Got: " << error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadDuplicateIdReturnsErrorWithBothLocations)
{
    AttributesCsvLoader loader = emerald_loader();

    auto result = loader.load(test_resources_dir / "duplicate_id.csv", kTilesetScope);
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
    AttributesCsvLoader loader = emerald_loader();

    auto result = loader.load(test_resources_dir / "unknown_behavior.csv", kTilesetScope);
    EXPECT_FALSE(result.has_value());

    std::string full_error_text = join_error_chain(result);
    EXPECT_TRUE(full_error_text.find("unknown behavior") != std::string::npos)
        << "Error should mention unknown behavior. Got: " << full_error_text;
    EXPECT_TRUE(full_error_text.find("MB_DOES_NOT_EXIST") != std::string::npos)
        << "Error should show the unknown behavior name. Got: " << full_error_text;
}

// The ProviderMap membership contract: has_provider() and map membership are equivalent. A provider-backed schema
// field with no provider in the map is an internal bug, so the loader panics instead of degrading to raw parsing.
TEST_F(AttributesCsvLoaderTest, ProviderBackedFieldMissingFromProviderMapPanics)
{
    ProviderMap missing_terrain = make_firered_provider_map();
    missing_terrain.erase("terrain");
    AttributesCsvLoader loader{&formatter_, &firered_schema_, &missing_terrain, &config_, &diag_};

    EXPECT_DEATH(
        std::ignore = loader.load(test_resources_dir / "valid_firered.csv", kTilesetScope), "no provider was built");
}

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvUnknownTerrainTypeReturnsError)
{
    AttributesCsvLoader loader = firered_loader();

    auto result = loader.load(test_resources_dir / "firered_unknown_terrain.csv", kTilesetScope);
    EXPECT_FALSE(result.has_value());

    std::string full_error_text = join_error_chain(result);
    EXPECT_TRUE(full_error_text.find("unknown terrain") != std::string::npos)
        << "Error should mention unknown terrain. Got: " << full_error_text;
    EXPECT_TRUE(full_error_text.find("TILE_TERRAIN_DOES_NOT_EXIST") != std::string::npos)
        << "Error should show the unknown terrain name. Got: " << full_error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvUnknownEncounterTypeReturnsError)
{
    AttributesCsvLoader loader = firered_loader();

    auto result = loader.load(test_resources_dir / "firered_unknown_encounter.csv", kTilesetScope);
    EXPECT_FALSE(result.has_value());

    std::string full_error_text = join_error_chain(result);
    EXPECT_TRUE(full_error_text.find("unknown encounter_type") != std::string::npos)
        << "Error should mention unknown encounter_type. Got: " << full_error_text;
    EXPECT_TRUE(full_error_text.find("TILE_ENCOUNTER_DOES_NOT_EXIST") != std::string::npos)
        << "Error should show the unknown encounter name. Got: " << full_error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvRowTooFewColumnsReturnsError)
{
    AttributesCsvLoader loader = firered_loader();

    auto result = loader.load(test_resources_dir / "firered_row_too_few_columns.csv", kTilesetScope);
    EXPECT_FALSE(result.has_value());

    std::string full_error_text = join_error_chain(result);
    EXPECT_TRUE(full_error_text.find("expected at least 8 columns") != std::string::npos)
        << "Error should mention expected 8 columns. Got: " << full_error_text;
}

// Raw fields have no provider cap, so the parser itself must reject a value the field cannot hold; otherwise the
// binary writer would silently mask it away later.
TEST_F(AttributesCsvLoaderTest, LoadFireredCsvRawFieldTooLargeReturnsError)
{
    AttributesCsvLoader loader = firered_loader();

    auto result = loader.load(test_resources_dir / "firered_raw_field_too_large.csv", kTilesetScope);
    EXPECT_FALSE(result.has_value());

    std::string full_error_text = join_error_chain(result);
    EXPECT_TRUE(full_error_text.find("attribute_2 value '99' exceeds the field's maximum of 15") != std::string::npos)
        << "Error should name the field, value, and maximum. Got: " << full_error_text;
}

TEST_F(AttributesCsvLoaderTest, LoadFireredCsvRawFieldNotIntegerReturnsError)
{
    AttributesCsvLoader loader = firered_loader();

    auto result = loader.load(test_resources_dir / "firered_raw_field_not_integer.csv", kTilesetScope);
    EXPECT_FALSE(result.has_value());

    std::string full_error_text = join_error_chain(result);
    EXPECT_TRUE(full_error_text.find("invalid attribute_2 value 'abc'") != std::string::npos)
        << "Error should name the field and the bad cell. Got: " << full_error_text;
}

// A wide raw field must accept values far beyond a 16-bit range. The widest legal field in a 4-byte word spans bits
// 0-28 (bits 29-30 are the structural layer_type, which no field may overlap). This regressed when the raw-cell parse
// bottomed out in std::stoi regardless of the requested integer type.
TEST_F(AttributesCsvLoaderTest, LoadWideRawFieldLargeValueSucceeds)
{
    auto schema_result = Schema::create({Field{"wide", 0x1FFFFFFF}}, 4);
    Schema wide_schema = std::move(schema_result).value();
    ProviderMap no_providers{};
    AttributesCsvLoader loader{&formatter_, &wide_schema, &no_providers, &config_, &diag_};

    auto result = loader.load(test_resources_dir / "wide_raw_field.csv", kTilesetScope);
    ASSERT_TRUE(result.has_value()) << join_error_chain(result);
    EXPECT_EQ(result.value().at(0).field("wide"), 500000000u);
}

TEST_F(AttributesCsvLoaderTest, LayerTypeColumnKnobOnAppliesFilledCellsAndLeavesBlankInferred)
{
    config_.write_layer_type_column = true;
    AttributesCsvLoader loader = emerald_loader();

    auto result = loader.load(test_resources_dir / "valid_layer_type.csv", kTilesetScope);
    ASSERT_TRUE(result.has_value());
    const auto &attributes = result.value();

    // Row 0: filled "covered" pins the layer type.
    ASSERT_TRUE(attributes.at(0).explicit_layer_type().has_value());
    EXPECT_EQ(attributes.at(0).explicit_layer_type().value(), LayerType::covered);
    EXPECT_EQ(attributes.at(0).layer_type(), LayerType::covered);

    // Row 1: blank cell leaves the layer type inferred (unset explicit, default normal).
    EXPECT_FALSE(attributes.at(1).explicit_layer_type().has_value());
    EXPECT_EQ(attributes.at(1).layer_type(), LayerType::normal);

    // Row 2: filled "split".
    ASSERT_TRUE(attributes.at(2).explicit_layer_type().has_value());
    EXPECT_EQ(attributes.at(2).explicit_layer_type().value(), LayerType::split);

    EXPECT_FALSE(diag_.warning_tag_counts().contains("layer-type-column"));
}

TEST_F(AttributesCsvLoaderTest, LayerTypeColumnKnobOnAppliesForMultiFieldSchema)
{
    config_.write_layer_type_column = true;
    AttributesCsvLoader loader = firered_loader();

    auto result = loader.load(test_resources_dir / "valid_firered_layer_type.csv", kTilesetScope);
    ASSERT_TRUE(result.has_value());
    const auto &attributes = result.value();

    ASSERT_TRUE(attributes.at(0).explicit_layer_type().has_value());
    EXPECT_EQ(attributes.at(0).explicit_layer_type().value(), LayerType::covered);
    EXPECT_FALSE(attributes.at(1).explicit_layer_type().has_value());
    EXPECT_EQ(attributes.at(2).explicit_layer_type().value(), LayerType::split);
}

TEST_F(AttributesCsvLoaderTest, LayerTypeColumnKnobOffWarnsOnceAndIgnoresValues)
{
    // Default MockInfraConfig has write_layer_type_column = false.
    AttributesCsvLoader loader = emerald_loader();

    auto result = loader.load(test_resources_dir / "valid_layer_type.csv", kTilesetScope);
    ASSERT_TRUE(result.has_value());
    const auto &attributes = result.value();

    // Values are ignored: nothing is pinned.
    EXPECT_FALSE(attributes.at(0).explicit_layer_type().has_value());
    EXPECT_EQ(attributes.at(0).layer_type(), LayerType::normal);

    // Exactly one warning for the whole file.
    ASSERT_TRUE(diag_.warning_tag_counts().contains("layer-type-column"));
    EXPECT_EQ(diag_.warning_tag_counts().at("layer-type-column"), 1u);
}

TEST_F(AttributesCsvLoaderTest, LayerTypeColumnKnobOnWithNoColumnNoWarning)
{
    config_.write_layer_type_column = true;
    AttributesCsvLoader loader = emerald_loader();

    auto result = loader.load(test_resources_dir / "valid.csv", kTilesetScope);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result.value().at(0).explicit_layer_type().has_value());
    EXPECT_FALSE(diag_.warning_tag_counts().contains("layer-type-column"));
}

TEST_F(AttributesCsvLoaderTest, LayerTypeColumnKnobOnBadTokenErrorsWithFileContext)
{
    config_.write_layer_type_column = true;
    AttributesCsvLoader loader = emerald_loader();

    auto result = loader.load(test_resources_dir / "invalid_layer_type_token.csv", kTilesetScope);
    ASSERT_FALSE(result.has_value());

    std::string full_error_text = join_error_chain(result);
    EXPECT_NE(full_error_text.find("invalid layerType"), std::string::npos) << full_error_text;
    EXPECT_NE(full_error_text.find("sideways"), std::string::npos) << full_error_text;
}

// The knob resolves under the scope passed to load(), which is how a paired primary's CSV (loaded during a secondary
// compile) resolves under the primary's name rather than the secondary's.
TEST_F(AttributesCsvLoaderTest, KnobResolvesUnderTheScopePassedToLoad)
{
    // A scope-sensitive config: the column is only enabled for "gTileset_Primary".
    class ScopedConfig final : public MockInfraConfig {
      protected:
        [[nodiscard]] ChainableResult<ConfigValue<bool>>
        write_layer_type_column_raw(ConfigScopeType, const std::string &scope) const override
        {
            return ConfigValue{
                scope == "gTileset_Primary", "Write Layer Type Column", "write_layer_type_column", "mock", {}};
        }
    };

    ScopedConfig scoped_config{};
    BufferedUserDiagnostics scoped_diag{};
    AttributesCsvLoader loader{&formatter_, &emerald_schema_, &emerald_providers_, &scoped_config, &scoped_diag};

    // Loading under the primary's scope applies the column.
    auto applied = loader.load(test_resources_dir / "valid_layer_type.csv", "gTileset_Primary");
    ASSERT_TRUE(applied.has_value());
    EXPECT_EQ(applied.value().at(0).explicit_layer_type().value(), LayerType::covered);

    // Loading the same file under a different scope ignores the column (and warns).
    auto ignored = loader.load(test_resources_dir / "valid_layer_type.csv", "gTileset_Other");
    ASSERT_TRUE(ignored.has_value());
    EXPECT_FALSE(ignored.value().at(0).explicit_layer_type().has_value());
    EXPECT_TRUE(scoped_diag.warning_tag_counts().contains("layer-type-column"));
}
