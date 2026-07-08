#include "porytiles/infra/config/metatile_attr_inference.hpp"

#include <algorithm>
#include <optional>

#include <gtest/gtest.h>

#include "porytiles/utilities/text/plain_text_formatter.hpp"

namespace porytiles {
namespace {

class MetatileAttrInferenceTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;

    [[nodiscard]] const MetatileAttrFieldSpec *find(const MetatileAttrFieldSpecs &specs, const std::string &name)
    {
        auto it =
            std::find_if(specs.begin(), specs.end(), [&](const MetatileAttrFieldSpec &s) { return s.name == name; });
        return it == specs.end() ? nullptr : &*it;
    }

    static InferenceEnumMember enum_member(const std::string &name, std::int64_t value)
    {
        return InferenceEnumMember{name, value};
    }
};

// Stock pokeemerald: mask defines only (behavior + layer), a 2-byte layout, behavior constants present.
TEST_F(MetatileAttrInferenceTest, EmeraldDefinesOnly)
{
    MetatileAttrScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}, {"METATILE_ATTR_LAYER_MASK", 0xF000}};
    scan.detected_attr_size = 2;
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attr_fields(scan, &formatter_);
    ASSERT_EQ(result.status, AttrInferenceStatus::valid);
    ASSERT_EQ(result.fields.size(), 1U); // layer_type is dropped
    const auto *behavior = find(result.fields, "behavior");
    ASSERT_NE(behavior, nullptr);
    EXPECT_EQ(behavior->mask.value(), 0x00FFU);
    EXPECT_FALSE(behavior->frlg_mask.has_value());
    ASSERT_TRUE(behavior->provider.has_value());
    EXPECT_EQ(behavior->provider->prefix, "MB_");
    EXPECT_TRUE(behavior->provider->skipped.contains("MB_INVALID"));

    // The layer-type mask is captured from METATILE_ATTR_LAYER_MASK even though layer_type is never a field.
    ASSERT_TRUE(result.layer_type_mask.has_value());
    EXPECT_EQ(result.layer_type_mask.value(), 0xF000U);
    EXPECT_FALSE(result.layer_type_frlg_mask.has_value());
}

// Stock pokefirered: the declaration enum plus the sMetatileAttrMasks table, single 4-byte layout.
TEST_F(MetatileAttrInferenceTest, FireredEnumPlusMaskTable)
{
    MetatileAttrScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_TERRAIN", 1),
        enum_member("METATILE_ATTRIBUTE_2", 2),
        enum_member("METATILE_ATTRIBUTE_3", 3),
        enum_member("METATILE_ATTRIBUTE_ENCOUNTER_TYPE", 4),
        enum_member("METATILE_ATTRIBUTE_5", 5),
        enum_member("METATILE_ATTRIBUTE_LAYER_TYPE", 6),
        enum_member("METATILE_ATTRIBUTE_7", 7),
        enum_member("METATILE_ATTRIBUTE_COUNT", 8),
        enum_member("METATILE_ATTRIBUTES_ALL", 255),
        // The value-name enums used by the terrain/encounter probes.
        enum_member("TILE_TERRAIN_NORMAL", 0),
        enum_member("TILE_ENCOUNTER_NONE", 0),
    };
    scan.masks_array = {
        {"METATILE_ATTRIBUTE_BEHAVIOR", 0x000001FF},
        {"METATILE_ATTRIBUTE_TERRAIN", 0x00003E00},
        {"METATILE_ATTRIBUTE_2", 0x0003C000},
        {"METATILE_ATTRIBUTE_3", 0x00FC0000},
        {"METATILE_ATTRIBUTE_ENCOUNTER_TYPE", 0x07000000},
        {"METATILE_ATTRIBUTE_5", 0x18000000},
        {"METATILE_ATTRIBUTE_LAYER_TYPE", 0x60000000},
        {"METATILE_ATTRIBUTE_7", 0x80000000},
    };
    scan.detected_attr_size = 4;
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attr_fields(scan, &formatter_);
    ASSERT_EQ(result.status, AttrInferenceStatus::valid);

    // Exactly the 7 non-layer fields, in enum declaration order.
    ASSERT_EQ(result.fields.size(), 7U);
    EXPECT_EQ(result.fields[0].name, "behavior");
    EXPECT_EQ(result.fields[1].name, "terrain");
    EXPECT_EQ(result.fields[2].name, "attribute_2");
    EXPECT_EQ(result.fields[3].name, "attribute_3");
    EXPECT_EQ(result.fields[4].name, "encounter_type");
    EXPECT_EQ(result.fields[5].name, "attribute_5");
    EXPECT_EQ(result.fields[6].name, "attribute_7");

    EXPECT_EQ(find(result.fields, "behavior")->mask.value(), 0x1FFU);
    EXPECT_EQ(find(result.fields, "terrain")->mask.value(), 0x3E00U);
    EXPECT_EQ(find(result.fields, "attribute_7")->mask.value(), 0x80000000U);

    // ATTRIBUTES_ALL and COUNT never become fields.
    EXPECT_EQ(find(result.fields, "attributes_all"), nullptr);

    // terrain probes TILE_TERRAIN_; encounter_type strips _TYPE and probes TILE_ENCOUNTER_.
    ASSERT_TRUE(find(result.fields, "terrain")->provider.has_value());
    EXPECT_EQ(find(result.fields, "terrain")->provider->prefix, "TILE_TERRAIN_");
    EXPECT_EQ(find(result.fields, "terrain")->provider->format, HeaderFormat::enums_only);
    ASSERT_TRUE(find(result.fields, "encounter_type")->provider.has_value());
    EXPECT_EQ(find(result.fields, "encounter_type")->provider->prefix, "TILE_ENCOUNTER_");

    // Numeric-suffix fields stay raw.
    EXPECT_FALSE(find(result.fields, "attribute_2")->provider.has_value());

    // The layer-type mask is captured from the sMetatileAttrMasks table (single 4-byte layout).
    ASSERT_TRUE(result.layer_type_mask.has_value());
    EXPECT_EQ(result.layer_type_mask.value(), 0x60000000U);
    EXPECT_FALSE(result.layer_type_frlg_mask.has_value());
}

// pokeemerald-expansion: dual layout. Emerald-side primary is behavior only; the rest are FRLG-only alternates.
TEST_F(MetatileAttrInferenceTest, ExpansionDualLayout)
{
    MetatileAttrScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_TERRAIN", 1),
        enum_member("METATILE_ATTRIBUTE_2", 2),
        enum_member("METATILE_ATTRIBUTE_3", 3),
        enum_member("METATILE_ATTRIBUTE_ENCOUNTER_TYPE", 4),
        enum_member("METATILE_ATTRIBUTE_5", 5),
        enum_member("METATILE_ATTRIBUTE_LAYER_TYPE", 6),
        enum_member("METATILE_ATTRIBUTE_7", 7),
        enum_member("METATILE_ATTRIBUTE_COUNT", 8),
        enum_member("TILE_TERRAIN_NORMAL", 0),
        enum_member("TILE_ENCOUNTER_NONE", 0),
    };
    scan.defines = {
        {"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF},
        {"METATILE_ATTR_LAYER_MASK", 0xF000},
        {"METATILE_ATTR_BEHAVIOR_MASK_FRLG", 0x000001FF},
        {"METATILE_ATTR_LAYER_MASK_FRLG", 0x60000000},
    };
    scan.masks_array = {
        {"METATILE_ATTRIBUTE_BEHAVIOR", 0x000001FF},
        {"METATILE_ATTRIBUTE_TERRAIN", 0x00003E00},
        {"METATILE_ATTRIBUTE_2", 0x0003C000},
        {"METATILE_ATTRIBUTE_3", 0x00FC0000},
        {"METATILE_ATTRIBUTE_ENCOUNTER_TYPE", 0x07000000},
        {"METATILE_ATTRIBUTE_5", 0x18000000},
        {"METATILE_ATTRIBUTE_LAYER_TYPE", 0x60000000},
        {"METATILE_ATTRIBUTE_7", 0x80000000},
    };
    scan.detected_attr_size = 2; // emerald-side primary schema
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attr_fields(scan, &formatter_);
    ASSERT_EQ(result.status, AttrInferenceStatus::valid);
    ASSERT_EQ(result.fields.size(), 7U);

    // behavior has a primary emerald mask AND an FRLG mask, plus the MB_ provider.
    const auto *behavior = find(result.fields, "behavior");
    ASSERT_NE(behavior, nullptr);
    EXPECT_EQ(behavior->mask.value(), 0x00FFU);
    ASSERT_TRUE(behavior->frlg_mask.has_value());
    EXPECT_EQ(behavior->frlg_mask.value(), 0x1FFU);
    ASSERT_TRUE(behavior->provider.has_value());

    // The other six are alternate-only: no primary mask, but an FRLG mask from the table.
    for (const char *name : {"terrain", "attribute_2", "attribute_3", "encounter_type", "attribute_5", "attribute_7"}) {
        const auto *field = find(result.fields, name);
        ASSERT_NE(field, nullptr) << name;
        EXPECT_FALSE(field->mask.has_value()) << name;
        EXPECT_TRUE(field->frlg_mask.has_value()) << name;
    }
    EXPECT_EQ(find(result.fields, "terrain")->frlg_mask.value(), 0x3E00U);

    // Dual layout: the primary layer mask is the emerald define, the FRLG layer mask is the FRLG define.
    ASSERT_TRUE(result.layer_type_mask.has_value());
    EXPECT_EQ(result.layer_type_mask.value(), 0xF000U);
    ASSERT_TRUE(result.layer_type_frlg_mask.has_value());
    EXPECT_EQ(result.layer_type_frlg_mask.value(), 0x60000000U);
}

// A base game whose LAYER_MASK is a custom value (not the size convention) has that exact value captured.
TEST_F(MetatileAttrInferenceTest, CustomLayerMaskCaptured)
{
    MetatileAttrScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}, {"METATILE_ATTR_LAYER_MASK", 0x0300}};
    scan.detected_attr_size = 2;
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attr_fields(scan, &formatter_);
    ASSERT_EQ(result.status, AttrInferenceStatus::valid);
    ASSERT_TRUE(result.layer_type_mask.has_value());
    EXPECT_EQ(result.layer_type_mask.value(), 0x0300U);
}

// A base game that declares no layer field at all leaves the captured layer mask unset, so the size convention
// fallback applies downstream.
TEST_F(MetatileAttrInferenceTest, AbsentLayerMaskLeavesNullopt)
{
    MetatileAttrScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_TERRAIN", 1),
        enum_member("METATILE_ATTRIBUTE_COUNT", 2),
        enum_member("TILE_TERRAIN_NORMAL", 0),
    };
    scan.masks_array = {
        {"METATILE_ATTRIBUTE_BEHAVIOR", 0x000000FF},
        {"METATILE_ATTRIBUTE_TERRAIN", 0x00000F00},
    };
    scan.detected_attr_size = 2;
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attr_fields(scan, &formatter_);
    ASSERT_EQ(result.status, AttrInferenceStatus::valid);
    EXPECT_FALSE(result.layer_type_mask.has_value());
    EXPECT_FALSE(result.layer_type_frlg_mask.has_value());
}

// A source with only behavior + layer defines but no behavior mask: the 2-byte exception fills behavior = 0x00FF.
TEST_F(MetatileAttrInferenceTest, TwoByteBehaviorOnlyException)
{
    MetatileAttrScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_LAYER_TYPE", 1),
        enum_member("METATILE_ATTRIBUTE_COUNT", 2),
    };
    scan.detected_attr_size = 2;
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attr_fields(scan, &formatter_);
    ASSERT_EQ(result.status, AttrInferenceStatus::valid);
    ASSERT_EQ(result.fields.size(), 1U);
    EXPECT_EQ(result.fields[0].name, "behavior");
    EXPECT_EQ(result.fields[0].mask.value(), 0x00FFU);
}

// A multi-field source where a declared field has no mask anywhere is a fatal, actionable error.
TEST_F(MetatileAttrInferenceTest, MultiFieldMissingMaskIsInvalid)
{
    MetatileAttrScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_TERRAIN", 1),
        enum_member("METATILE_ATTRIBUTE_LAYER_TYPE", 2),
        enum_member("METATILE_ATTRIBUTE_COUNT", 3),
    };
    scan.masks_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 0x1FF}}; // terrain has no mask
    scan.detected_attr_size = 4;
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attr_fields(scan, &formatter_);
    ASSERT_EQ(result.status, AttrInferenceStatus::invalid);
    EXPECT_NE(result.error_message.find("terrain"), std::string::npos);
    // The diagnostic names all three escape hatches.
    EXPECT_NE(result.error_message.find("sMetatileAttrMasks"), std::string::npos);
    EXPECT_NE(result.error_message.find("METATILE_ATTR_TERRAIN_MASK"), std::string::npos);
    EXPECT_NE(result.error_message.find("metatile_attr_field_overrides"), std::string::npos);
}

// LAYER (without _TYPE) normalizes to LAYER_TYPE and is therefore dropped like the canonical spelling.
TEST_F(MetatileAttrInferenceTest, LayerNormalization)
{
    MetatileAttrScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}, {"METATILE_ATTR_LAYER_MASK", 0xF000}};
    scan.detected_attr_size = 2;
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attr_fields(scan, &formatter_);
    ASSERT_EQ(result.status, AttrInferenceStatus::valid);
    ASSERT_EQ(result.fields.size(), 1U);
    EXPECT_EQ(result.fields[0].name, "behavior");
    EXPECT_EQ(find(result.fields, "layer_type"), nullptr);
    EXPECT_EQ(find(result.fields, "layer"), nullptr);
}

// When the behaviors header has no entries, the behavior field is emitted without a provider, plus a warning.
TEST_F(MetatileAttrInferenceTest, EmptyBehaviorsHeaderFallsBackToRaw)
{
    MetatileAttrScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}, {"METATILE_ATTR_LAYER_MASK", 0xF000}};
    scan.detected_attr_size = 2;
    scan.behaviors_header_present = false;

    const auto result = infer_metatile_attr_fields(scan, &formatter_);
    ASSERT_EQ(result.status, AttrInferenceStatus::valid);
    ASSERT_EQ(result.fields.size(), 1U);
    EXPECT_FALSE(result.fields[0].provider.has_value());
    EXPECT_FALSE(result.warnings.empty());
}

// A shift table entry that disagrees with the mask offset produces a warning but does not fail.
TEST_F(MetatileAttrInferenceTest, ShiftMismatchWarns)
{
    MetatileAttrScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_LAYER_TYPE", 1),
        enum_member("METATILE_ATTRIBUTE_COUNT", 2),
    };
    scan.masks_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 0x00FF}};
    scan.shifts_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 4}}; // wrong: 0x00FF has offset 0
    scan.detected_attr_size = 2;
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attr_fields(scan, &formatter_);
    ASSERT_EQ(result.status, AttrInferenceStatus::valid);
    EXPECT_EQ(result.fields[0].mask.value(), 0x00FFU); // mask wins
    EXPECT_FALSE(result.warnings.empty());
}

// Single layout: when a mask define and the mask table disagree, the define wins and a warning is emitted.
TEST_F(MetatileAttrInferenceTest, SingleLayoutDefineVsTableConflictDefineWins)
{
    MetatileAttrScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_LAYER_TYPE", 1),
        enum_member("METATILE_ATTRIBUTE_COUNT", 2),
    };
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
    scan.masks_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 0x01FF}}; // disagrees with the define
    scan.detected_attr_size = 2;
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attr_fields(scan, &formatter_);
    ASSERT_EQ(result.status, AttrInferenceStatus::valid);
    EXPECT_EQ(find(result.fields, "behavior")->mask.value(), 0x00FFU); // define wins
    EXPECT_FALSE(result.warnings.empty());
}

// Dual layout: when the FRLG define and the FRLG-valued table disagree, the define wins and a warning is emitted.
TEST_F(MetatileAttrInferenceTest, DualLayoutFrlgDefineVsTableConflictDefineWins)
{
    MetatileAttrScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_LAYER_TYPE", 1),
        enum_member("METATILE_ATTRIBUTE_COUNT", 2),
    };
    scan.defines = {
        {"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF},
        {"METATILE_ATTR_BEHAVIOR_MASK_FRLG", 0x01FF},
        {"METATILE_ATTR_LAYER_MASK", 0xF000},
        {"METATILE_ATTR_LAYER_MASK_FRLG", 0x60000000},
    };
    scan.masks_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 0x03FF}}; // disagrees with the FRLG define
    scan.detected_attr_size = 2;
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attr_fields(scan, &formatter_);
    ASSERT_EQ(result.status, AttrInferenceStatus::valid);
    const auto *behavior = find(result.fields, "behavior");
    ASSERT_NE(behavior, nullptr);
    EXPECT_EQ(behavior->mask.value(), 0x00FFU);
    EXPECT_EQ(behavior->frlg_mask.value(), 0x1FFU); // FRLG define wins over the table
    EXPECT_FALSE(result.warnings.empty());
}

// Nothing attribute-related at all means the provider should defer to the next provider.
TEST_F(MetatileAttrInferenceTest, NothingFoundIsNotProvided)
{
    MetatileAttrScan scan;
    scan.enum_members = {enum_member("SOME_UNRELATED_ENUM_MEMBER", 0)};
    scan.detected_attr_size = 2;

    const auto result = infer_metatile_attr_fields(scan, &formatter_);
    EXPECT_EQ(result.status, AttrInferenceStatus::not_provided);
    EXPECT_TRUE(result.fields.empty());
}

} // namespace
} // namespace porytiles
