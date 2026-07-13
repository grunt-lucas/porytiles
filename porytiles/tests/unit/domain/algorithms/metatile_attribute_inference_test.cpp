#include "porytiles/domain/algorithms/metatile_attribute_inference.hpp"

#include <algorithm>
#include <optional>

#include <gtest/gtest.h>

#include "porytiles/utilities/text/plain_text_formatter.hpp"

namespace porytiles {
namespace {

class MetatileAttributeInferenceTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;

    [[nodiscard]] const MetatileAttributeFieldSpec *
    find(const MetatileAttributeFieldSpecs &specs, const std::string &name)
    {
        auto it = std::find_if(
            specs.begin(), specs.end(), [&](const MetatileAttributeFieldSpec &s) { return s.name == name; });
        return it == specs.end() ? nullptr : &*it;
    }

    static InferenceEnumMember enum_member(const std::string &name, std::int64_t value)
    {
        return InferenceEnumMember{name, value};
    }
};

// Stock pokeemerald: mask defines only (behavior + layer), behavior constants present. One candidate set whose
// required width follows from the widest mask (the 0xF000 layer mask needs 2 bytes).
TEST_F(MetatileAttributeInferenceTest, EmeraldDefinesOnly)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}, {"METATILE_ATTR_LAYER_MASK", 0xF000}};
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    const auto &candidate = result.candidates.front();
    EXPECT_EQ(candidate.required_bytes, 2U);
    EXPECT_FALSE(candidate.synthesized);

    ASSERT_EQ(candidate.fields.size(), 1U); // layer_type is dropped
    const auto *behavior = find(candidate.fields, "behavior");
    ASSERT_NE(behavior, nullptr);
    EXPECT_EQ(behavior->mask.value(), 0x00FFU);
    ASSERT_TRUE(behavior->provider.has_value());
    EXPECT_EQ(behavior->provider->prefix, "MB_");
    EXPECT_TRUE(behavior->provider->skipped.contains("MB_INVALID"));

    // The layer-type mask is captured from METATILE_ATTR_LAYER_MASK even though layer_type is never a field.
    ASSERT_TRUE(candidate.layer_type_mask.has_value());
    EXPECT_EQ(candidate.layer_type_mask.value(), 0xF000U);
}

// Stock pokefirered: the declaration enum plus the sMetatileAttrMasks table. One candidate set requiring 4 bytes.
TEST_F(MetatileAttributeInferenceTest, FireredEnumPlusMaskTable)
{
    MetatileAttributeScan scan;
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
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    const auto &candidate = result.candidates.front();
    EXPECT_EQ(candidate.required_bytes, 4U);
    EXPECT_FALSE(candidate.synthesized);

    // Exactly the 7 non-layer fields, in enum declaration order.
    ASSERT_EQ(candidate.fields.size(), 7U);
    EXPECT_EQ(candidate.fields[0].name, "behavior");
    EXPECT_EQ(candidate.fields[1].name, "terrain");
    EXPECT_EQ(candidate.fields[2].name, "attribute_2");
    EXPECT_EQ(candidate.fields[3].name, "attribute_3");
    EXPECT_EQ(candidate.fields[4].name, "encounter_type");
    EXPECT_EQ(candidate.fields[5].name, "attribute_5");
    EXPECT_EQ(candidate.fields[6].name, "attribute_7");

    EXPECT_EQ(find(candidate.fields, "behavior")->mask.value(), 0x1FFU);
    EXPECT_EQ(find(candidate.fields, "terrain")->mask.value(), 0x3E00U);
    EXPECT_EQ(find(candidate.fields, "attribute_7")->mask.value(), 0x80000000U);

    // ATTRIBUTES_ALL and COUNT never become fields.
    EXPECT_EQ(find(candidate.fields, "attributes_all"), nullptr);

    // terrain probes TILE_TERRAIN_; encounter_type strips _TYPE and probes TILE_ENCOUNTER_.
    ASSERT_TRUE(find(candidate.fields, "terrain")->provider.has_value());
    EXPECT_EQ(find(candidate.fields, "terrain")->provider->prefix, "TILE_TERRAIN_");
    EXPECT_EQ(find(candidate.fields, "terrain")->provider->format, HeaderFormat::enums_only);
    ASSERT_TRUE(find(candidate.fields, "encounter_type")->provider.has_value());
    EXPECT_EQ(find(candidate.fields, "encounter_type")->provider->prefix, "TILE_ENCOUNTER_");

    // Numeric-suffix fields stay raw.
    EXPECT_FALSE(find(candidate.fields, "attribute_2")->provider.has_value());

    // The layer-type mask is captured from the sMetatileAttrMasks table.
    ASSERT_TRUE(candidate.layer_type_mask.has_value());
    EXPECT_EQ(candidate.layer_type_mask.value(), 0x60000000U);
}

// pokeemerald-expansion: both mask layouts declared. Two candidate sets: the bare defines (behavior-only, 2 bytes)
// and the FRLG defines plus table (seven fields, 4 bytes).
TEST_F(MetatileAttributeInferenceTest, ExpansionDualLayout)
{
    MetatileAttributeScan scan;
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
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 2U);

    // The bare-define set comes first: behavior only, emerald layer mask, 2 bytes.
    const auto &bare = result.candidates[0];
    EXPECT_EQ(bare.required_bytes, 2U);
    ASSERT_EQ(bare.fields.size(), 1U);
    EXPECT_EQ(bare.fields[0].name, "behavior");
    EXPECT_EQ(bare.fields[0].mask.value(), 0x00FFU);
    ASSERT_TRUE(bare.fields[0].provider.has_value());
    ASSERT_TRUE(bare.layer_type_mask.has_value());
    EXPECT_EQ(bare.layer_type_mask.value(), 0xF000U);

    // The FRLG set: seven fields (FRLG define for behavior, table for the rest), FRLG layer mask, 4 bytes.
    const auto &frlg = result.candidates[1];
    EXPECT_EQ(frlg.required_bytes, 4U);
    ASSERT_EQ(frlg.fields.size(), 7U);
    EXPECT_EQ(find(frlg.fields, "behavior")->mask.value(), 0x1FFU);
    EXPECT_EQ(find(frlg.fields, "terrain")->mask.value(), 0x3E00U);
    EXPECT_EQ(find(frlg.fields, "attribute_7")->mask.value(), 0x80000000U);
    ASSERT_TRUE(frlg.layer_type_mask.has_value());
    EXPECT_EQ(frlg.layer_type_mask.value(), 0x60000000U);
}

// A base game whose LAYER_MASK is a custom value (not the size-based default) has that exact value captured.
TEST_F(MetatileAttributeInferenceTest, CustomLayerMaskCaptured)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}, {"METATILE_ATTR_LAYER_MASK", 0x0300}};
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    ASSERT_TRUE(result.candidates.front().layer_type_mask.has_value());
    EXPECT_EQ(result.candidates.front().layer_type_mask.value(), 0x0300U);
    // 0x0300 reaches bit 10, so the set still requires 2 bytes.
    EXPECT_EQ(result.candidates.front().required_bytes, 2U);
}

// A base game that declares no layer field at all leaves the captured layer mask unset, so the size-based default
// fallback applies downstream.
TEST_F(MetatileAttributeInferenceTest, AbsentLayerMaskLeavesNullopt)
{
    MetatileAttributeScan scan;
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
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    EXPECT_FALSE(result.candidates.front().layer_type_mask.has_value());
    EXPECT_EQ(result.candidates.front().required_bytes, 2U); // 0x0F00 reaches bit 12
}

// A source declaring exactly BEHAVIOR and LAYER_TYPE with no masks anywhere completes as one synthesized two-byte
// candidate (behavior = 0x00FF). The gate is structural: with no masks there is nothing to infer a size from, and
// every real stock project with this shape is the two-byte emerald family.
TEST_F(MetatileAttributeInferenceTest, TwoByteBehaviorOnlyException)
{
    MetatileAttributeScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_LAYER_TYPE", 1),
        enum_member("METATILE_ATTRIBUTE_COUNT", 2),
    };
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    const auto &candidate = result.candidates.front();
    EXPECT_TRUE(candidate.synthesized);
    EXPECT_EQ(candidate.required_bytes, 2U);
    ASSERT_EQ(candidate.fields.size(), 1U);
    EXPECT_EQ(candidate.fields[0].name, "behavior");
    EXPECT_EQ(candidate.fields[0].mask.value(), 0x00FFU);
    EXPECT_FALSE(candidate.layer_type_mask.has_value());
}

// A multi-field source where a declared field has no mask anywhere is a fatal, actionable error.
TEST_F(MetatileAttributeInferenceTest, MultiFieldMissingMaskIsInvalid)
{
    MetatileAttributeScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_TERRAIN", 1),
        enum_member("METATILE_ATTRIBUTE_LAYER_TYPE", 2),
        enum_member("METATILE_ATTRIBUTE_COUNT", 3),
    };
    scan.masks_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 0x1FF}}; // terrain has no mask
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::invalid);
    EXPECT_NE(result.error_message.find("terrain"), std::string::npos);
    // The diagnostic names all three escape hatches.
    EXPECT_NE(result.error_message.find("sMetatileAttrMasks"), std::string::npos);
    EXPECT_NE(result.error_message.find("METATILE_ATTR_TERRAIN_MASK"), std::string::npos);
    EXPECT_NE(result.error_message.find("metatile_attribute_field_overrides"), std::string::npos);
}

// LAYER (without _TYPE) normalizes to LAYER_TYPE and is therefore dropped like the canonical spelling.
TEST_F(MetatileAttributeInferenceTest, LayerNormalization)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}, {"METATILE_ATTR_LAYER_MASK", 0xF000}};
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    const auto &fields = result.candidates.front().fields;
    ASSERT_EQ(fields.size(), 1U);
    EXPECT_EQ(fields[0].name, "behavior");
    EXPECT_EQ(find(fields, "layer_type"), nullptr);
    EXPECT_EQ(find(fields, "layer"), nullptr);
}

// When the behaviors header has no entries, the behavior field is emitted without a provider, plus a warning.
TEST_F(MetatileAttributeInferenceTest, EmptyBehaviorsHeaderFallsBackToRaw)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}, {"METATILE_ATTR_LAYER_MASK", 0xF000}};
    scan.behaviors_header_present = false;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    ASSERT_EQ(result.candidates.front().fields.size(), 1U);
    EXPECT_FALSE(result.candidates.front().fields[0].provider.has_value());
    EXPECT_FALSE(result.warnings.empty());
}

// A shift table entry that disagrees with the mask offset produces a warning but does not fail.
TEST_F(MetatileAttributeInferenceTest, ShiftMismatchWarns)
{
    MetatileAttributeScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_LAYER_TYPE", 1),
        enum_member("METATILE_ATTRIBUTE_COUNT", 2),
    };
    scan.masks_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 0x00FF}};
    scan.shifts_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 4}}; // wrong: 0x00FF has offset 0
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    EXPECT_EQ(result.candidates.front().fields[0].mask.value(), 0x00FFU); // mask wins
    EXPECT_FALSE(result.warnings.empty());
}

// Single layout: when a mask define and the mask table disagree, the define wins and a warning is emitted.
TEST_F(MetatileAttributeInferenceTest, SingleLayoutDefineVsTableConflictDefineWins)
{
    MetatileAttributeScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_LAYER_TYPE", 1),
        enum_member("METATILE_ATTRIBUTE_COUNT", 2),
    };
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
    scan.masks_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 0x01FF}}; // disagrees with the define
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    EXPECT_EQ(find(result.candidates.front().fields, "behavior")->mask.value(), 0x00FFU); // define wins
    EXPECT_FALSE(result.warnings.empty());
}

// Dual layout: when the FRLG define and the FRLG-valued table disagree, the define wins and a warning is emitted.
TEST_F(MetatileAttributeInferenceTest, DualLayoutFrlgDefineVsTableConflictDefineWins)
{
    MetatileAttributeScan scan;
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
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 2U);
    EXPECT_EQ(find(result.candidates[0].fields, "behavior")->mask.value(), 0x00FFU);
    EXPECT_EQ(find(result.candidates[1].fields, "behavior")->mask.value(), 0x1FFU); // FRLG define wins over the table
    EXPECT_FALSE(result.warnings.empty());
}

TEST_F(MetatileAttributeInferenceTest, AmbiguousConditionalMaskIsInvalid)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
    scan.ambiguous_defines.insert("METATILE_ATTR_BEHAVIOR_MASK");
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    EXPECT_EQ(result.status, AttributeInferenceStatus::invalid);
    EXPECT_NE(result.error_message.find("METATILE_ATTR_BEHAVIOR_MASK"), std::string::npos);
    EXPECT_NE(result.error_message.find("metatile_attribute_field_overrides"), std::string::npos);
}

// Nothing attribute-related at all means the provider should defer to the next provider.
TEST_F(MetatileAttributeInferenceTest, NothingFoundIsNotProvided)
{
    MetatileAttributeScan scan;
    scan.enum_members = {enum_member("SOME_UNRELATED_ENUM_MEMBER", 0)};

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    EXPECT_EQ(result.status, AttributeInferenceStatus::not_provided);
    EXPECT_TRUE(result.candidates.empty());
}

// A single one-byte mask yields a one-byte candidate: required_bytes follows the widest mask, not a floor of 2.
TEST_F(MetatileAttributeInferenceTest, OneByteMaskYieldsOneByteCandidate)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x0F}};
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    EXPECT_EQ(result.candidates.front().required_bytes, 1U);
    EXPECT_FALSE(result.candidates.front().layer_type_mask.has_value());
}

// The declaration width maps from the scanned element type of struct Tileset's metatileAttributes member: u8/u16/u32
// map to 1/2/4, anything else stays nullopt so the downstream "match the attribute size" default applies.
TEST_F(MetatileAttributeInferenceTest, DeclarationSizeMapsFromElementType)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
    scan.behaviors_header_present = true;

    scan.attributes_element_type = "u8";
    EXPECT_EQ(infer_metatile_attribute_candidates(scan, &formatter_).declaration_size, 1U);
    scan.attributes_element_type = "u16";
    EXPECT_EQ(infer_metatile_attribute_candidates(scan, &formatter_).declaration_size, 2U);
    scan.attributes_element_type = "u32";
    EXPECT_EQ(infer_metatile_attribute_candidates(scan, &formatter_).declaration_size, 4U);
    scan.attributes_element_type = "u64";
    EXPECT_EQ(infer_metatile_attribute_candidates(scan, &formatter_).declaration_size, std::nullopt);
    scan.attributes_element_type = std::nullopt;
    EXPECT_EQ(infer_metatile_attribute_candidates(scan, &formatter_).declaration_size, std::nullopt);
}

// The declaration width is a project fact independent of the mask layout, so it survives every status: an invalid
// inference (ambiguous conditional define) and a not-provided inference both still carry it.
TEST_F(MetatileAttributeInferenceTest, DeclarationSizeSurvivesInvalidAndNotProvided)
{
    MetatileAttributeScan invalid_scan;
    invalid_scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
    invalid_scan.ambiguous_defines.insert("METATILE_ATTR_BEHAVIOR_MASK");
    invalid_scan.attributes_element_type = "u16";
    const auto invalid_result = infer_metatile_attribute_candidates(invalid_scan, &formatter_);
    ASSERT_EQ(invalid_result.status, AttributeInferenceStatus::invalid);
    EXPECT_EQ(invalid_result.declaration_size, 2U);

    MetatileAttributeScan empty_scan;
    empty_scan.attributes_element_type = "u32";
    const auto empty_result = infer_metatile_attribute_candidates(empty_scan, &formatter_);
    ASSERT_EQ(empty_result.status, AttributeInferenceStatus::not_provided);
    EXPECT_EQ(empty_result.declaration_size, 4U);
}

} // namespace
} // namespace porytiles
