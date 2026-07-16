#include "gtest/gtest.h"

#include <cstdint>
#include <format>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"

using namespace porytiles;

namespace {

// Builds the field carrying FieldRole::layer_type at the given mask.
Field layer_type_field(std::uint32_t mask)
{
    return Field{"layer_type", mask, 0, std::nullopt, FieldRole::layer_type};
}

// The stock firered value-backed fields plus the layer_type-role field at bits 29-30.
std::vector<Field> firered_fields()
{
    std::vector<Field> fields;
    fields.emplace_back("behavior", 0x000001FF);
    fields.emplace_back("terrain", 0x00003E00);
    fields.emplace_back("attribute_2", 0x0003C000);
    fields.emplace_back("attribute_3", 0x00FC0000);
    fields.emplace_back("encounter_type", 0x07000000);
    fields.emplace_back("attribute_5", 0x18000000);
    fields.push_back(layer_type_field(0x60000000));
    fields.emplace_back("attribute_7", 0x80000000);
    return fields;
}

} // namespace

TEST(MetatileAttributeSchemaTest, FieldDerivations)
{
    const Field behavior{"behavior", 0x1FF};
    EXPECT_EQ(behavior.offset(), 0u);
    EXPECT_EQ(behavior.width(), 9u);
    EXPECT_EQ(behavior.max_value(), 511u);

    const Field terrain{"terrain", 0x3E00};
    EXPECT_EQ(terrain.offset(), 9u);
    EXPECT_EQ(terrain.width(), 5u);
    EXPECT_EQ(terrain.max_value(), 31u);

    const Field layer{"layer", 0x60000000};
    EXPECT_EQ(layer.offset(), 29u);
    EXPECT_EQ(layer.width(), 2u);
    EXPECT_EQ(layer.max_value(), 3u);

    const Field top_bit{"top_bit", 0x80000000};
    EXPECT_EQ(top_bit.offset(), 31u);
    EXPECT_EQ(top_bit.width(), 1u);
    EXPECT_EQ(top_bit.max_value(), 1u);
}

TEST(MetatileAttributeSchemaTest, FieldProviderPresence)
{
    const Field no_provider{"behavior", 0x1FF};
    EXPECT_FALSE(no_provider.has_provider());

    ProviderDefinition definition{};
    definition.header = "include/constants/metatile_behaviors.h";
    definition.prefix = "MB_";
    const Field with_provider{"behavior", 0x1FF, 0, definition};
    ASSERT_TRUE(with_provider.has_provider());
    EXPECT_EQ(with_provider.provider_definition().prefix, "MB_");
    EXPECT_EQ(with_provider.provider_definition().format, HeaderFormat::either);
    EXPECT_TRUE(with_provider.provider_definition().skipped.empty());
}

TEST(MetatileAttributeSchemaTest, FieldRolePresence)
{
    const Field plain{"behavior", 0x1FF};
    EXPECT_FALSE(plain.role().has_value());
    EXPECT_FALSE(plain.packs_layer_type());

    const Field role_field = layer_type_field(0x60000000);
    ASSERT_TRUE(role_field.role().has_value());
    EXPECT_EQ(role_field.role().value(), FieldRole::layer_type);
    EXPECT_TRUE(role_field.packs_layer_type());
}

TEST(MetatileAttributeSchemaTest, ToEnumDefinitionCopiesDescriptionAndStoresArgs)
{
    ProviderDefinition definition{};
    definition.header = "include/global.fieldmap.h";
    definition.prefix = "TILE_TERRAIN_";
    definition.skipped = {"TILE_TERRAIN_UNUSED"};
    definition.format = HeaderFormat::enums_only;

    const EnumDefinition enum_definition = definition.to_enum_definition("terrain", 0x1F);

    // The description fields carry over verbatim.
    EXPECT_EQ(enum_definition.prefix, "TILE_TERRAIN_");
    EXPECT_EQ(enum_definition.skipped, definition.skipped);
    EXPECT_EQ(enum_definition.format, HeaderFormat::enums_only);

    // The two facts that live on the owning field are the supplied arguments.
    EXPECT_EQ(enum_definition.max_value, 0x1Fu);
    EXPECT_EQ(enum_definition.field_display_name, "terrain");
}

TEST(MetatileAttributeSchemaTest, ToEnumDefinitionDefaultsAreCarriedOver)
{
    ProviderDefinition definition{};
    definition.prefix = "MB_";

    const EnumDefinition enum_definition = definition.to_enum_definition("behavior", 0xFFFF);

    EXPECT_EQ(enum_definition.prefix, "MB_");
    EXPECT_TRUE(enum_definition.skipped.empty());
    EXPECT_EQ(enum_definition.format, HeaderFormat::either);
    EXPECT_EQ(enum_definition.max_value, 0xFFFFu);
    EXPECT_EQ(enum_definition.field_display_name, "behavior");
}

TEST(MetatileAttributeSchemaTest, CreateAcceptsFireredLayout)
{
    auto result = Schema::create(firered_fields(), 4);
    ASSERT_TRUE(result.has_value());

    const Schema &schema = result.value();
    EXPECT_EQ(schema.attribute_bytes(), 4u);

    ASSERT_EQ(schema.fields().size(), 8u);
    EXPECT_EQ(schema.fields()[0].name(), "behavior");
    EXPECT_EQ(schema.fields()[1].name(), "terrain");
    EXPECT_EQ(schema.fields()[2].name(), "attribute_2");
    EXPECT_EQ(schema.fields()[3].name(), "attribute_3");
    EXPECT_EQ(schema.fields()[4].name(), "encounter_type");
    EXPECT_EQ(schema.fields()[5].name(), "attribute_5");
    EXPECT_EQ(schema.fields()[6].name(), "layer_type");
    EXPECT_EQ(schema.fields()[7].name(), "attribute_7");
}

TEST(MetatileAttributeSchemaTest, ValueFieldsExcludeTheLayerTypeRoleField)
{
    auto result = Schema::create(firered_fields(), 4);
    ASSERT_TRUE(result.has_value());

    const Schema &schema = result.value();
    ASSERT_EQ(schema.value_fields().size(), 7u);
    for (const Field &field : schema.value_fields()) {
        EXPECT_FALSE(field.packs_layer_type());
        EXPECT_NE(field.name(), "layer_type");
    }
    // Schema order is preserved around the excluded role field.
    EXPECT_EQ(schema.value_fields()[5].name(), "attribute_5");
    EXPECT_EQ(schema.value_fields()[6].name(), "attribute_7");
}

TEST(MetatileAttributeSchemaTest, LayerTypeAccessorsDeriveFromTheRoleField)
{
    std::vector<Field> two_byte_fields;
    two_byte_fields.emplace_back("behavior", 0x00FF);
    two_byte_fields.push_back(layer_type_field(0xF000));
    auto two_byte_result = Schema::create(std::move(two_byte_fields), 2);
    ASSERT_TRUE(two_byte_result.has_value());
    ASSERT_NE(two_byte_result.value().layer_type_field(), nullptr);
    EXPECT_EQ(two_byte_result.value().layer_type_field()->name(), "layer_type");
    EXPECT_EQ(two_byte_result.value().layer_type_mask(), 0x0000F000u);
    EXPECT_EQ(two_byte_result.value().layer_type_offset(), 12u);

    std::vector<Field> four_byte_fields;
    four_byte_fields.emplace_back("behavior", 0x000001FF);
    four_byte_fields.push_back(layer_type_field(0x60000000));
    auto four_byte_result = Schema::create(std::move(four_byte_fields), 4);
    ASSERT_TRUE(four_byte_result.has_value());
    EXPECT_EQ(four_byte_result.value().layer_type_mask(), 0x60000000u);
    EXPECT_EQ(four_byte_result.value().layer_type_offset(), 29u);
}

TEST(MetatileAttributeSchemaTest, AbsentLayerTypeFieldDisablesLayerType)
{
    // No role field means no layer bits: the mask reads 0 and the bits it would have used are free for
    // ordinary fields.
    std::vector<Field> fields;
    fields.emplace_back("behavior", 0x00FF);
    fields.emplace_back("wide", 0xF000);
    auto result = Schema::create(std::move(fields), 2);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().layer_type_field(), nullptr);
    EXPECT_EQ(result.value().layer_type_mask(), 0U);
    EXPECT_EQ(result.value().value_fields().size(), 2u);
}

TEST(MetatileAttributeSchemaTest, LayerTypeRoleFieldValidatedLikeAnyField)
{
    PlainTextFormatter formatter;

    // Non-contiguous role mask fails the ordinary contiguity rule.
    std::vector<Field> gapped;
    gapped.emplace_back("behavior", 0x000001FF);
    gapped.push_back(layer_type_field(0x50000000));
    auto gapped_result = Schema::create(std::move(gapped), 4);
    ASSERT_FALSE(gapped_result.has_value());
    EXPECT_NE(gapped_result.error().join(formatter).find("layer_type"), std::string::npos);
    EXPECT_NE(gapped_result.error().join(formatter).find("contiguous"), std::string::npos);

    // A role mask beyond the attribute size fails the ordinary range rule.
    std::vector<Field> beyond;
    beyond.emplace_back("behavior", 0x00FF);
    beyond.push_back(layer_type_field(0x30000));
    auto beyond_result = Schema::create(std::move(beyond), 2);
    ASSERT_FALSE(beyond_result.has_value());
    EXPECT_NE(beyond_result.error().join(formatter).find("layer_type"), std::string::npos);
    EXPECT_NE(beyond_result.error().join(formatter).find("extends beyond"), std::string::npos);

    // A field overlapping the role field fails the ordinary overlap rule.
    std::vector<Field> overlapping;
    overlapping.push_back(layer_type_field(0x0300));
    overlapping.emplace_back("intruder", 0x0300);
    auto overlap_result = Schema::create(std::move(overlapping), 2);
    ASSERT_FALSE(overlap_result.has_value());
    const std::string message = overlap_result.error().join(formatter);
    EXPECT_NE(message.find("intruder"), std::string::npos);
    EXPECT_NE(message.find("layer_type"), std::string::npos);
}

TEST(MetatileAttributeSchemaTest, RelocatedLayerTypeRoleFieldHonored)
{
    // The role field sits wherever its mask says, with no positional convention.
    std::vector<Field> fields;
    fields.emplace_back("behavior", 0x00FF);
    fields.push_back(layer_type_field(0x0300));
    auto result = Schema::create(std::move(fields), 2);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().layer_type_mask(), 0x0300U);
    EXPECT_EQ(result.value().layer_type_offset(), 8u);
}

TEST(MetatileAttributeSchemaTest, RejectsSecondLayerTypeRoleField)
{
    PlainTextFormatter formatter;
    std::vector<Field> fields;
    fields.push_back(layer_type_field(0xF000));
    fields.push_back(Field{"other_layer", 0x0F00, 0, std::nullopt, FieldRole::layer_type});
    auto result = Schema::create(std::move(fields), 2);
    ASSERT_FALSE(result.has_value());
    const std::string message = result.error().join(formatter);
    EXPECT_NE(message.find("other_layer"), std::string::npos);
    EXPECT_NE(message.find("layer_type"), std::string::npos);
}

TEST(MetatileAttributeSchemaTest, RejectsProviderOnLayerTypeRoleField)
{
    PlainTextFormatter formatter;
    ProviderDefinition definition{};
    definition.header = "include/global.fieldmap.h";
    definition.prefix = "TILE_LAYER_";
    std::vector<Field> fields;
    fields.push_back(Field{"layer_type", 0xF000, 0, definition, FieldRole::layer_type});
    auto result = Schema::create(std::move(fields), 2);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().join(formatter).find("provider"), std::string::npos);
}

TEST(MetatileAttributeSchemaTest, RejectsDefaultOnLayerTypeRoleField)
{
    PlainTextFormatter formatter;
    std::vector<Field> fields;
    fields.push_back(Field{"layer_type", 0xF000, 2, std::nullopt, FieldRole::layer_type});
    auto result = Schema::create(std::move(fields), 2);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().join(formatter).find("default"), std::string::npos);
}

TEST(MetatileAttributeSchemaTest, RejectsLayerTypeNameWithoutTheRole)
{
    // The attributes CSV detects its trailing pin column by this exact name, so a plain value field cannot
    // use it.
    PlainTextFormatter formatter;
    std::vector<Field> fields;
    fields.emplace_back("layer_type", 0xF000);
    auto result = Schema::create(std::move(fields), 2);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().join(formatter).find("reserved"), std::string::npos);
}

TEST(MetatileAttributeSchemaTest, CreateAcceptsWidestMasksAroundTheRoleField)
{
    // Fields may fill every bit not taken by the role field.
    std::vector<Field> two_byte_full;
    two_byte_full.emplace_back("all", 0x0FFF);
    two_byte_full.push_back(layer_type_field(0xF000));
    auto two_byte_result = Schema::create(std::move(two_byte_full), 2);
    ASSERT_TRUE(two_byte_result.has_value());

    std::vector<Field> four_byte_full;
    four_byte_full.emplace_back("low", 0x1FFFFFFF);
    four_byte_full.push_back(layer_type_field(0x60000000));
    four_byte_full.emplace_back("top", 0x80000000);
    auto four_byte_result = Schema::create(std::move(four_byte_full), 4);
    ASSERT_TRUE(four_byte_result.has_value());
}

TEST(MetatileAttributeSchemaTest, RejectsZeroMask)
{
    PlainTextFormatter formatter;
    std::vector<Field> fields;
    fields.emplace_back("mystery", 0x0);

    auto result = Schema::create(std::move(fields), 2);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().join(formatter).find("mystery"), std::string::npos);
}

TEST(MetatileAttributeSchemaTest, RejectsNonContiguousMask)
{
    PlainTextFormatter formatter;

    std::vector<Field> gapped;
    gapped.emplace_back("gapped", 0xF0F0);
    auto gapped_result = Schema::create(std::move(gapped), 2);
    ASSERT_FALSE(gapped_result.has_value());
    EXPECT_NE(gapped_result.error().join(formatter).find("gapped"), std::string::npos);

    std::vector<Field> scattered;
    scattered.emplace_back("scattered", 0x5);
    auto scattered_result = Schema::create(std::move(scattered), 2);
    ASSERT_FALSE(scattered_result.has_value());
    EXPECT_NE(scattered_result.error().join(formatter).find("scattered"), std::string::npos);
}

TEST(MetatileAttributeSchemaTest, RejectsOverlappingMasks)
{
    PlainTextFormatter formatter;
    std::vector<Field> fields;
    fields.emplace_back("behavior", 0x1FF);
    fields.emplace_back("intruder", 0x100);

    auto result = Schema::create(std::move(fields), 2);
    ASSERT_FALSE(result.has_value());

    const std::string message = result.error().join(formatter);
    EXPECT_NE(message.find("intruder"), std::string::npos);
    EXPECT_NE(message.find("behavior"), std::string::npos);
}

TEST(MetatileAttributeSchemaTest, RejectsMaskBeyondAttributeSize)
{
    PlainTextFormatter formatter;

    std::vector<Field> beyond;
    beyond.emplace_back("beyond", 0x30000);
    auto beyond_result = Schema::create(std::move(beyond), 2);
    ASSERT_FALSE(beyond_result.has_value());
    EXPECT_NE(beyond_result.error().join(formatter).find("beyond"), std::string::npos);

    std::vector<Field> top_of_two;
    top_of_two.emplace_back("top_of_two", 0x8000);
    EXPECT_TRUE(Schema::create(std::move(top_of_two), 2).has_value());

    std::vector<Field> top_of_four;
    top_of_four.emplace_back("top_of_four", 0x80000000);
    EXPECT_TRUE(Schema::create(std::move(top_of_four), 4).has_value());
}

TEST(MetatileAttributeSchemaTest, RejectsDefaultThatDoesNotFit)
{
    PlainTextFormatter formatter;

    std::vector<Field> low_ok;
    low_ok.emplace_back("low", 0x3, 3);
    EXPECT_TRUE(Schema::create(std::move(low_ok), 2).has_value());

    std::vector<Field> low_bad;
    low_bad.emplace_back("low", 0x3, 4);
    auto low_bad_result = Schema::create(std::move(low_bad), 2);
    ASSERT_FALSE(low_bad_result.has_value());
    EXPECT_NE(low_bad_result.error().join(formatter).find("low"), std::string::npos);

    // A shifted mask has the same 2-bit width, so the fit check must be width-based (max_value), not raw mask.
    std::vector<Field> shifted_ok;
    shifted_ok.emplace_back("shifted", 0x18000000, 3);
    EXPECT_TRUE(Schema::create(std::move(shifted_ok), 4).has_value());

    std::vector<Field> shifted_bad;
    shifted_bad.emplace_back("shifted", 0x18000000, 4);
    auto shifted_bad_result = Schema::create(std::move(shifted_bad), 4);
    ASSERT_FALSE(shifted_bad_result.has_value());
    EXPECT_NE(shifted_bad_result.error().join(formatter).find("shifted"), std::string::npos);
}

TEST(MetatileAttributeSchemaTest, RejectsDuplicateName)
{
    PlainTextFormatter formatter;
    std::vector<Field> fields;
    fields.emplace_back("behavior", 0x00FF);
    fields.emplace_back("behavior", 0xFF00);

    auto result = Schema::create(std::move(fields), 2);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().join(formatter).find("behavior"), std::string::npos);
}

TEST(MetatileAttributeSchemaTest, OneByteSchemaAcceptsALayerTypeRoleField)
{
    std::vector<Field> fields;
    fields.emplace_back("behavior", 0x0F);
    fields.push_back(layer_type_field(0x30));
    auto result = Schema::create(std::move(fields), 1);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().attribute_bytes(), 1u);
    EXPECT_EQ(result.value().layer_type_mask(), 0x30U);
    EXPECT_EQ(result.value().layer_type_offset(), 4u);
}

TEST(MetatileAttributeSchemaTest, HeaderFormatToString)
{
    EXPECT_EQ(to_string(HeaderFormat::defines_only), "defines-only");
    EXPECT_EQ(to_string(HeaderFormat::enums_only), "enums-only");
    EXPECT_EQ(to_string(HeaderFormat::either), "either");
}

TEST(MetatileAttributeSchemaTest, HeaderFormatFormatsAndStreams)
{
    EXPECT_EQ(std::format("{}", HeaderFormat::defines_only), "defines-only");

    std::ostringstream stream;
    stream << HeaderFormat::enums_only;
    EXPECT_EQ(stream.str(), "enums-only");
}

TEST(MetatileAttributeSchemaTest, FieldRoleToStringFormatsAndStreams)
{
    EXPECT_EQ(to_string(FieldRole::layer_type), "layer_type");
    EXPECT_EQ(std::format("{}", FieldRole::layer_type), "layer_type");

    std::ostringstream stream;
    stream << FieldRole::layer_type;
    EXPECT_EQ(stream.str(), "layer_type");
}
