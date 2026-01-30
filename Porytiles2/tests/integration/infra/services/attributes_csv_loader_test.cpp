#include "gtest/gtest.h"

#include <filesystem>
#include <unordered_map>

#include "porytiles2/domain/models/layer.hpp"
#include "porytiles2/domain/services/behavior_map_provider.hpp"
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

class AttributesCsvLoaderTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_{};
    StubBehaviorMapProvider behavior_map_{};
};

} // namespace

// =============================================================================
// Valid CSV Tests
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
// File Error Tests
// =============================================================================

TEST_F(AttributesCsvLoaderTest, LoadNonExistentFileReturnsError)
{
    AttributesCsvLoader loader{&formatter_, &behavior_map_};

    auto result = loader.load(kTestResourcesDir / "does_not_exist.csv");
    EXPECT_FALSE(result.has_value());

    std::string error_text = result.chain().back()->join(formatter_);
    EXPECT_TRUE(error_text.find("not found") != std::string::npos)
        << "Error should mention file not found. Got: " << error_text;
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
