#include "gtest/gtest.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <vector>

#include "porytiles/domain/models/layer.hpp"
#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/infra/algorithms/porymap_artifact_parsers.hpp"

using namespace porytiles;

namespace {

// The stock emerald shape: a single behavior field in a 2-byte attribute (layer_type is structural, bits 12-15).
Schema make_emerald_schema()
{
    auto result = Schema::create({Field{"behavior", 0x00FF}}, 2);
    return std::move(result).value();
}

// The stock firered shape: seven fields in a 4-byte attribute (layer_type is structural, bits 29-30).
Schema make_firered_schema()
{
    auto result = Schema::create(
        {
            Field{"behavior", 0x000001FF},
            Field{"terrain", 0x00003E00},
            Field{"attribute_2", 0x0003C000},
            Field{"attribute_3", 0x00FC0000},
            Field{"encounter_type", 0x07000000},
            Field{"attribute_5", 0x18000000},
            Field{"attribute_7", 0x80000000},
        },
        4);
    return std::move(result).value();
}

// A deliberately non-stock schema: fields at unusual offsets, a raw field, and a nonzero-default field, in a 4-byte
// word. Proves the parser/emitter pair works for arbitrary masks, not just the stock layouts.
Schema make_custom_schema()
{
    auto result = Schema::create(
        {
            Field{"low_nibble", 0x0000000F},
            Field{"mid_field", 0x0007F800},    // bits 11-18
            Field{"defaulted", 0x00300000, 2}, // bits 20-21, nonzero default
            Field{"top_bit", 0x80000000},      // bit 31
        },
        4);
    return std::move(result).value();
}

// A 2-byte schema with the layer type disabled (explicit mask 0): layer bits are neither written nor read.
Schema make_layer_disabled_schema()
{
    auto result = Schema::create({Field{"behavior", 0x00FF}}, 2, std::optional<std::uint32_t>{0U});
    return std::move(result).value();
}

// A 1-byte schema (behavior in bits 0-3, terrain in bits 4-5). The layer type is disabled by the 1-byte convention.
Schema make_one_byte_schema()
{
    auto result = Schema::create({Field{"behavior", 0x0F}, Field{"terrain", 0x30}}, 1);
    return std::move(result).value();
}

void write_bytes(const std::filesystem::path &path, const std::vector<std::uint8_t> &bytes)
{
    std::ofstream out{path, std::ios::binary};
    for (const auto byte : bytes) {
        out << byte;
    }
    out.flush();
}

[[nodiscard]] std::vector<std::uint8_t> read_bytes(const std::filesystem::path &path)
{
    std::ifstream in{path, std::ios::binary};
    return {std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
}

// Encodes one 4-byte firered-format attribute word into little-endian bytes.
[[nodiscard]] std::vector<std::uint8_t> le32(std::uint32_t value)
{
    return {
        static_cast<std::uint8_t>(value),
        static_cast<std::uint8_t>(value >> 8),
        static_cast<std::uint8_t>(value >> 16),
        static_cast<std::uint8_t>(value >> 24)};
}

[[nodiscard]] std::vector<std::uint8_t> le16(std::uint16_t value)
{
    return {static_cast<std::uint8_t>(value), static_cast<std::uint8_t>(value >> 8)};
}

[[nodiscard]] std::vector<std::uint8_t> concat(std::vector<std::uint8_t> a, const std::vector<std::uint8_t> &b)
{
    a.insert(a.end(), b.begin(), b.end());
    return a;
}

} // namespace

class MetatileAttributeParserTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        test_dir_ = std::filesystem::temp_directory_path() / "porytiles_metatile_attribute_parser_test";
        std::filesystem::create_directories(test_dir_);
        test_file_ = test_dir_ / "metatile_attributes.bin";
    }

    void TearDown() override
    {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::filesystem::path test_dir_;
    std::filesystem::path test_file_;
};

TEST_F(MetatileAttributeParserTest, ParsesFireredLayoutFields)
{
    const Schema schema = make_firered_schema();
    // behavior=0x1AB, terrain=17, attribute_2=9, attribute_3=33, encounter=5, attribute_5=2, layer=split(2),
    // attribute_7=1
    const std::uint32_t raw =
        (0x1ABu) | (17u << 9) | (9u << 14) | (33u << 18) | (5u << 24) | (2u << 27) | (2u << 29) | (1u << 31);
    write_bytes(test_file_, le32(raw));

    auto result = parse_metatile_attributes(test_file_, schema);
    ASSERT_TRUE(result.has_value()) << "Parse failed";
    ASSERT_EQ(result.value().size(), 1);

    const auto &attribute = result.value()[0];
    EXPECT_EQ(attribute.field(attribute::field_behavior), 0x1ABu);
    EXPECT_EQ(attribute.field(attribute::field_terrain), 17u);
    EXPECT_EQ(attribute.field(attribute::field_attribute_2), 9u);
    EXPECT_EQ(attribute.field(attribute::field_attribute_3), 33u);
    EXPECT_EQ(attribute.field(attribute::field_encounter_type), 5u);
    EXPECT_EQ(attribute.field(attribute::field_attribute_5), 2u);
    EXPECT_EQ(attribute.layer_type(), LayerType::split);
    EXPECT_EQ(attribute.field(attribute::field_attribute_7), 1u);
}

TEST_F(MetatileAttributeParserTest, ParsesEmeraldLayoutFields)
{
    const Schema schema = make_emerald_schema();
    // Attribute 0: behavior=0x42, layer=normal(0). Attribute 1: behavior=0xFF, layer=covered(1).
    write_bytes(test_file_, concat(le16(0x0042), le16(static_cast<std::uint16_t>(0x00FF | (1u << 12)))));

    auto result = parse_metatile_attributes(test_file_, schema);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 2);

    EXPECT_EQ(result.value()[0].field(attribute::field_behavior), 0x42u);
    EXPECT_EQ(result.value()[0].layer_type(), LayerType::normal);
    EXPECT_EQ(result.value()[1].field(attribute::field_behavior), 0xFFu);
    EXPECT_EQ(result.value()[1].layer_type(), LayerType::covered);
}

TEST_F(MetatileAttributeParserTest, SaveWritesExactBytesForCustomSchema)
{
    const Schema schema = make_custom_schema();

    MetatileAttribute attribute{};
    attribute.field("low_nibble", 0xA);
    attribute.field("mid_field", 0x55);
    attribute.field("defaulted", 1);
    attribute.field("top_bit", 1);
    attribute.layer_type(LayerType::split); // structural bits 29-30 = 2

    auto save_result = save_metatile_attributes_bin({attribute}, test_file_, schema);
    ASSERT_TRUE(save_result.has_value());

    const std::uint32_t expected = 0xAu | (0x55u << 11) | (1u << 20) | (1u << 31) | (2u << 29);
    EXPECT_EQ(read_bytes(test_file_), le32(expected));
}

TEST_F(MetatileAttributeParserTest, AbsentFieldSavesItsSchemaDefault)
{
    const Schema schema = make_custom_schema();

    // 'defaulted' (default 2) is never stored, so the emitted word must carry the default, matching the effective
    // value the attributes CSV renders for the same attribute.
    MetatileAttribute attribute{};
    attribute.field("low_nibble", 0x3);

    auto save_result = save_metatile_attributes_bin({attribute}, test_file_, schema);
    ASSERT_TRUE(save_result.has_value());

    const std::uint32_t expected = 0x3u | (2u << 20);
    EXPECT_EQ(read_bytes(test_file_), le32(expected));
}

TEST_F(MetatileAttributeParserTest, CustomSchemaRoundTripsThroughSaveAndParse)
{
    const Schema schema = make_custom_schema();

    MetatileAttribute attribute_0{};
    attribute_0.field("low_nibble", 0xF);
    attribute_0.field("mid_field", 0xFF);
    attribute_0.field("defaulted", 3);
    attribute_0.field("top_bit", 0);
    attribute_0.layer_type(LayerType::covered);

    MetatileAttribute attribute_1{};
    attribute_1.field("low_nibble", 0);
    attribute_1.field("mid_field", 0);
    attribute_1.field("defaulted", 0); // explicit 0 beats the nonzero default and must survive the round trip
    attribute_1.field("top_bit", 1);
    attribute_1.layer_type(LayerType::normal);

    ASSERT_TRUE(save_metatile_attributes_bin({attribute_0, attribute_1}, test_file_, schema).has_value());

    auto result = parse_metatile_attributes(test_file_, schema);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 2);

    const auto &parsed_0 = result.value()[0];
    EXPECT_EQ(parsed_0.field("low_nibble"), 0xFu);
    EXPECT_EQ(parsed_0.field("mid_field"), 0xFFu);
    EXPECT_EQ(parsed_0.field("defaulted"), 3u);
    EXPECT_EQ(parsed_0.field("top_bit"), 0u);
    EXPECT_EQ(parsed_0.layer_type(), LayerType::covered);

    const auto &parsed_1 = result.value()[1];
    EXPECT_EQ(parsed_1.field("low_nibble"), 0u);
    EXPECT_EQ(parsed_1.field("mid_field"), 0u);
    EXPECT_EQ(parsed_1.field("defaulted"), 0u);
    EXPECT_EQ(parsed_1.field("top_bit"), 1u);
    EXPECT_EQ(parsed_1.layer_type(), LayerType::normal);
}

TEST_F(MetatileAttributeParserTest, EmeraldLayoutRoundTripsThroughSaveAndParse)
{
    const Schema schema = make_emerald_schema();

    MetatileAttribute attribute{};
    attribute.field(attribute::field_behavior, 0x21);
    attribute.layer_type(LayerType::split);

    ASSERT_TRUE(save_metatile_attributes_bin({attribute}, test_file_, schema).has_value());

    auto result = parse_metatile_attributes(test_file_, schema);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].field(attribute::field_behavior), 0x21u);
    EXPECT_EQ(result.value()[0].layer_type(), LayerType::split);
}

TEST_F(MetatileAttributeParserTest, DisabledLayerTypeRoundTripsAsNormal)
{
    const Schema schema = make_layer_disabled_schema();
    ASSERT_EQ(schema.layer_type_mask(), 0u);

    MetatileAttribute attribute{};
    attribute.field(attribute::field_behavior, 0x21);
    attribute.layer_type(LayerType::split); // dropped when packing, because the layer type is disabled

    ASSERT_TRUE(save_metatile_attributes_bin({attribute}, test_file_, schema).has_value());
    // Only the behavior byte is written; no layer-type bits appear.
    EXPECT_EQ(read_bytes(test_file_), le16(0x0021));

    auto result = parse_metatile_attributes(test_file_, schema);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].field(attribute::field_behavior), 0x21u);
    EXPECT_EQ(result.value()[0].layer_type(), LayerType::normal);
}

TEST_F(MetatileAttributeParserTest, DisabledLayerTypeIgnoresUnusedHighBits)
{
    const Schema schema = make_layer_disabled_schema();
    // Bits 12-15 = 3 would be an invalid layer type if enabled, but a disabled layer type never reads them.
    write_bytes(test_file_, le16(static_cast<std::uint16_t>(0x0007 | (3u << 12))));

    auto result = parse_metatile_attributes(test_file_, schema);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    EXPECT_EQ(result.value()[0].field(attribute::field_behavior), 0x07u);
    EXPECT_EQ(result.value()[0].layer_type(), LayerType::normal);
}

TEST_F(MetatileAttributeParserTest, OneByteSchemaRoundTrips)
{
    const Schema schema = make_one_byte_schema();
    ASSERT_EQ(schema.attribute_bytes(), 1u);

    MetatileAttribute a0{};
    a0.field("behavior", 0x5);
    a0.field("terrain", 0x2);
    MetatileAttribute a1{};
    a1.field("behavior", 0xF);
    a1.field("terrain", 0x3);

    ASSERT_TRUE(save_metatile_attributes_bin({a0, a1}, test_file_, schema).has_value());
    // Each attribute is one byte: behavior in bits 0-3, terrain in bits 4-5.
    EXPECT_EQ(read_bytes(test_file_), (std::vector<std::uint8_t>{0x25, 0x3F}));

    auto result = parse_metatile_attributes(test_file_, schema);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 2);
    EXPECT_EQ(result.value()[0].field("behavior"), 0x5u);
    EXPECT_EQ(result.value()[0].field("terrain"), 0x2u);
    EXPECT_EQ(result.value()[1].field("behavior"), 0xFu);
    EXPECT_EQ(result.value()[1].field("terrain"), 0x3u);
}

TEST_F(MetatileAttributeParserTest, ErrorOnSizeNotMultipleOfAttributeBytes)
{
    // 5 bytes is not a multiple of the firered schema's 4-byte width.
    write_bytes(test_file_, {0x00, 0x00, 0x00, 0x00, 0x01});

    auto result = parse_metatile_attributes(test_file_, make_firered_schema());
    ASSERT_FALSE(result.has_value());
}

TEST_F(MetatileAttributeParserTest, ErrorOnInvalidLayerTypeBits)
{
    // Layer type bits 12-15 hold 3, which is not a valid LayerType in the 2-byte layout.
    write_bytes(test_file_, le16(static_cast<std::uint16_t>(3u << 12)));

    auto result = parse_metatile_attributes(test_file_, make_emerald_schema());
    ASSERT_FALSE(result.has_value());
}

TEST_F(MetatileAttributeParserTest, ErrorOnMissingFile)
{
    auto result = parse_metatile_attributes(test_dir_ / "nonexistent.bin", make_firered_schema());
    ASSERT_FALSE(result.has_value());
}

TEST_F(MetatileAttributeParserTest, EmptyFileReturnsEmptyVector)
{
    write_bytes(test_file_, {});

    auto result = parse_metatile_attributes(test_file_, make_firered_schema());
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}
