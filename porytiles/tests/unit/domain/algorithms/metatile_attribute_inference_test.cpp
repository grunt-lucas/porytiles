#include "porytiles/domain/algorithms/metatile_attribute_inference.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

namespace porytiles {
namespace {

class MetatileAttributeInferenceTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;
    BufferedUserDiagnostics diag_;

    // True when some buffered warning line contains the given text. Warnings are buffered as line vectors, so the
    // probe has to walk both levels.
    [[nodiscard]] bool warned_about(const std::string &text) const
    {
        return std::any_of(diag_.warnings().begin(), diag_.warnings().end(), [&](const std::vector<std::string> &w) {
            return std::any_of(
                w.begin(), w.end(), [&](const std::string &line) { return line.find(text) != std::string::npos; });
        });
    }

    [[nodiscard]] const MetatileAttributeFieldDefinition *
    find(const MetatileAttributeFieldDefinitions &definitions, const std::string &name)
    {
        auto it = std::find_if(definitions.begin(), definitions.end(), [&](const MetatileAttributeFieldDefinition &s) {
            return s.name == name;
        });
        return it == definitions.end() ? nullptr : &*it;
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
    scan.header_source = "include/global.fieldmap.h";

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    const auto &candidate = result.candidates.front();
    EXPECT_EQ(candidate.required_bytes, 2U);

    // Defines only: the header is the sole mask source, and no absent table is named.
    EXPECT_EQ(candidate.origin, "the METATILE_ATTR_*_MASK defines");
    EXPECT_EQ(candidate.source, "include/global.fieldmap.h");

    ASSERT_EQ(candidate.fields.size(), 2U);
    const auto *behavior = find(candidate.fields, "behavior");
    ASSERT_NE(behavior, nullptr);
    EXPECT_EQ(behavior->mask.value(), 0x00FFU);
    EXPECT_FALSE(behavior->role.has_value());
    ASSERT_TRUE(behavior->provider.has_value());
    EXPECT_EQ(behavior->provider->prefix, "MB_");
    EXPECT_TRUE(behavior->provider->skipped.contains("MB_INVALID"));

    // METATILE_ATTR_LAYER_MASK becomes an ordinary field carrying the layer_type role, with no provider.
    const auto *layer = find(candidate.fields, "layer_type");
    ASSERT_NE(layer, nullptr);
    EXPECT_EQ(layer->mask.value(), 0xF000U);
    EXPECT_EQ(layer->role, FieldRole::layer_type);
    EXPECT_FALSE(layer->provider.has_value());
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
    scan.header_source = "include/global.fieldmap.h";
    scan.masks_table_source = "src/fieldmap.c";

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    const auto &candidate = result.candidates.front();
    EXPECT_EQ(candidate.required_bytes, 4U);

    // Every mask came from the table, so the set names only the table's file. The header holds the enum, but it
    // declares no mask, and the width provenance downstream must not point users at it.
    EXPECT_EQ(candidate.origin, "the sMetatileAttrMasks table");
    EXPECT_EQ(candidate.source, "src/fieldmap.c");

    // All 8 fields in enum declaration order, layer_type among them at its declared position.
    ASSERT_EQ(candidate.fields.size(), 8U);
    EXPECT_EQ(candidate.fields[0].name, "behavior");
    EXPECT_EQ(candidate.fields[1].name, "terrain");
    EXPECT_EQ(candidate.fields[2].name, "attribute_2");
    EXPECT_EQ(candidate.fields[3].name, "attribute_3");
    EXPECT_EQ(candidate.fields[4].name, "encounter_type");
    EXPECT_EQ(candidate.fields[5].name, "attribute_5");
    EXPECT_EQ(candidate.fields[6].name, "layer_type");
    EXPECT_EQ(candidate.fields[7].name, "attribute_7");

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

    // The layer_type field's mask comes from the sMetatileAttrMasks table, and it carries the role.
    const auto *layer = find(candidate.fields, "layer_type");
    ASSERT_NE(layer, nullptr);
    EXPECT_EQ(layer->mask.value(), 0x60000000U);
    EXPECT_EQ(layer->role, FieldRole::layer_type);
    EXPECT_FALSE(layer->provider.has_value());
}

// pokeemerald-expansion: both mask layouts declared. Two candidate sets: the bare defines (behavior + layer_type,
// 2 bytes) and the FRLG defines plus table (eight fields, 4 bytes).
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
    scan.header_source = "include/global.fieldmap.h";
    scan.masks_table_source = "src/fieldmap.c";

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 2U);

    // The bare-define set comes first: behavior plus the emerald layer_type role field, 2 bytes.
    const auto &bare = result.candidates[0];
    EXPECT_EQ(bare.required_bytes, 2U);
    // The bare set draws on the header alone; the FRLG set draws on both files and lists them in prose order.
    EXPECT_EQ(bare.source, "include/global.fieldmap.h");
    EXPECT_EQ(result.candidates[1].source, "include/global.fieldmap.h, src/fieldmap.c");
    ASSERT_EQ(bare.fields.size(), 2U);
    EXPECT_EQ(bare.fields[0].name, "behavior");
    EXPECT_EQ(bare.fields[0].mask.value(), 0x00FFU);
    ASSERT_TRUE(bare.fields[0].provider.has_value());
    const auto *bare_layer = find(bare.fields, "layer_type");
    ASSERT_NE(bare_layer, nullptr);
    EXPECT_EQ(bare_layer->mask.value(), 0xF000U);
    EXPECT_EQ(bare_layer->role, FieldRole::layer_type);

    // The FRLG set: eight fields (FRLG defines for behavior and layer_type, table for the rest), 4 bytes.
    const auto &frlg = result.candidates[1];
    EXPECT_EQ(frlg.required_bytes, 4U);
    ASSERT_EQ(frlg.fields.size(), 8U);
    EXPECT_EQ(find(frlg.fields, "behavior")->mask.value(), 0x1FFU);
    EXPECT_EQ(find(frlg.fields, "terrain")->mask.value(), 0x3E00U);
    EXPECT_EQ(find(frlg.fields, "attribute_7")->mask.value(), 0x80000000U);
    const auto *frlg_layer = find(frlg.fields, "layer_type");
    ASSERT_NE(frlg_layer, nullptr);
    EXPECT_EQ(frlg_layer->mask.value(), 0x60000000U);
    EXPECT_EQ(frlg_layer->role, FieldRole::layer_type);
}

// A project with both define flavors but no mask table must not claim a table it never read: the FRLG set names the
// defines only, and its source list holds just the header.
TEST_F(MetatileAttributeInferenceTest, DualLayoutWithoutAMaskTableNamesOnlyTheDefines)
{
    MetatileAttributeScan scan;
    scan.defines = {
        {"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF},
        {"METATILE_ATTR_LAYER_MASK", 0xF000},
        {"METATILE_ATTR_BEHAVIOR_MASK_FRLG", 0x000001FF},
        {"METATILE_ATTR_LAYER_MASK_FRLG", 0x60000000},
    };
    scan.behaviors_header_present = true;
    scan.header_source = "include/global.fieldmap.h";

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 2U);
    EXPECT_EQ(result.candidates[1].origin, "the METATILE_ATTR_*_MASK_FRLG defines");
    EXPECT_EQ(result.candidates[1].source, "include/global.fieldmap.h");
}

// A partially recorded scan lists only the paths it has. The origin still names both sources, since both really did
// contribute masks, but the unrecorded one contributes no path rather than an empty entry.
TEST_F(MetatileAttributeInferenceTest, PartiallyRecordedScanPathsListOnlyWhatIsKnown)
{
    MetatileAttributeScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_LAYER_TYPE", 1),
        enum_member("METATILE_ATTRIBUTE_COUNT", 2),
    };
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
    scan.masks_array = {{"METATILE_ATTRIBUTE_LAYER_TYPE", 0x0000F000}};
    scan.behaviors_header_present = true;
    scan.header_source = "include/global.fieldmap.h";

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    EXPECT_EQ(result.candidates.front().origin, "the METATILE_ATTR_*_MASK defines and the sMetatileAttrMasks table");
    EXPECT_EQ(result.candidates.front().source, "include/global.fieldmap.h");
}

// A scan that recorded no paths yields candidates with no source, so the reconciler can drop the parenthetical
// instead of printing an empty one.
TEST_F(MetatileAttributeInferenceTest, UnrecordedScanPathsYieldNoCandidateSource)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}, {"METATILE_ATTR_LAYER_MASK", 0xF000}};
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    EXPECT_TRUE(result.candidates.front().source.empty());
}

// A base game whose LAYER_MASK is a custom value (not the vanilla position) has that exact value captured on the
// role field.
TEST_F(MetatileAttributeInferenceTest, CustomLayerMaskCaptured)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}, {"METATILE_ATTR_LAYER_MASK", 0x0300}};
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    const auto *layer = find(result.candidates.front().fields, "layer_type");
    ASSERT_NE(layer, nullptr);
    EXPECT_EQ(layer->mask.value(), 0x0300U);
    EXPECT_EQ(layer->role, FieldRole::layer_type);
    // 0x0300 reaches bit 10, so the set still requires 2 bytes.
    EXPECT_EQ(result.candidates.front().required_bytes, 2U);
}

// A base game that declares no layer field at all yields a candidate with no layer_type role field, which disables
// the layer type downstream.
TEST_F(MetatileAttributeInferenceTest, AbsentLayerFieldYieldsNoRoleField)
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

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    EXPECT_EQ(find(result.candidates.front().fields, "layer_type"), nullptr);
    for (const auto &field : result.candidates.front().fields) {
        EXPECT_FALSE(field.role.has_value());
    }
    EXPECT_EQ(result.candidates.front().required_bytes, 2U); // 0x0F00 reaches bit 12
}

// A source declaring exactly BEHAVIOR and LAYER_TYPE with no masks anywhere is a fatal, actionable error. The old
// synthesized two-byte completion is gone: either the masks are readable or the user declares the layout.
TEST_F(MetatileAttributeInferenceTest, BehaviorOnlyWithNoMasksIsInvalid)
{
    MetatileAttributeScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_LAYER_TYPE", 1),
        enum_member("METATILE_ATTRIBUTE_COUNT", 2),
    };
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::invalid);
    EXPECT_NE(result.error_message.find("behavior"), std::string::npos);
    EXPECT_NE(result.error_message.find("METATILE_ATTR_BEHAVIOR_MASK"), std::string::npos);
    EXPECT_NE(result.error_message.find("metatile_attribute_fields"), std::string::npos);
}

// The layer_type field is covered by the missing-mask check like any other field, and its suggested define follows
// the emerald family's METATILE_ATTR_LAYER_MASK spelling rather than the normalized LAYER_TYPE suffix.
TEST_F(MetatileAttributeInferenceTest, MissingLayerMaskIsInvalidWithEmeraldDefineSpelling)
{
    MetatileAttributeScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_LAYER_TYPE", 1),
        enum_member("METATILE_ATTRIBUTE_COUNT", 2),
    };
    scan.masks_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 0x00FF}}; // layer_type has no mask anywhere
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::invalid);
    EXPECT_NE(result.error_message.find("layer_type"), std::string::npos);
    EXPECT_NE(result.error_message.find("METATILE_ATTR_LAYER_MASK"), std::string::npos);
    EXPECT_EQ(result.error_message.find("METATILE_ATTR_LAYER_TYPE_MASK"), std::string::npos);
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

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::invalid);
    EXPECT_NE(result.error_message.find("terrain"), std::string::npos);
    // The diagnostic names all three escape hatches.
    EXPECT_NE(result.error_message.find("sMetatileAttrMasks"), std::string::npos);
    EXPECT_NE(result.error_message.find("METATILE_ATTR_TERRAIN_MASK"), std::string::npos);
    EXPECT_NE(result.error_message.find("metatile_attribute_field_overrides"), std::string::npos);
}

// LAYER (without _TYPE) normalizes to LAYER_TYPE, so METATILE_ATTR_LAYER_MASK yields a field named layer_type
// carrying the role, never a plain field named "layer".
TEST_F(MetatileAttributeInferenceTest, LayerNormalization)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}, {"METATILE_ATTR_LAYER_MASK", 0xF000}};
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    const auto &fields = result.candidates.front().fields;
    ASSERT_EQ(fields.size(), 2U);
    EXPECT_EQ(fields[0].name, "behavior");
    const auto *layer = find(fields, "layer_type");
    ASSERT_NE(layer, nullptr);
    EXPECT_EQ(layer->role, FieldRole::layer_type);
    EXPECT_EQ(find(fields, "layer"), nullptr);
}

// When the behaviors header has no entries, the behavior field is emitted without a provider, plus a warning.
TEST_F(MetatileAttributeInferenceTest, EmptyBehaviorsHeaderFallsBackToRaw)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}, {"METATILE_ATTR_LAYER_MASK", 0xF000}};
    scan.behaviors_header_present = false;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    ASSERT_EQ(result.candidates.front().fields.size(), 2U); // behavior plus the layer_type role field
    EXPECT_EQ(result.candidates.front().fields[0].name, "behavior");
    EXPECT_FALSE(result.candidates.front().fields[0].provider.has_value());
    EXPECT_EQ(diag_.warning_tag_counts().at(metatile_attr_inference_tag), 1U);
    EXPECT_TRUE(warned_about("no behavior constants found in"));
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
    scan.masks_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 0x00FF}, {"METATILE_ATTRIBUTE_LAYER_TYPE", 0xF000}};
    scan.shifts_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 4}}; // wrong: 0x00FF has offset 0
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    EXPECT_EQ(result.candidates.front().fields[0].mask.value(), 0x00FFU); // mask wins
    EXPECT_EQ(diag_.warning_tag_counts().at(metatile_attr_inference_tag), 1U);
    EXPECT_TRUE(warned_about("shift table entry (4) does not match its mask offset (0)"));
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
    scan.masks_array = {
        {"METATILE_ATTRIBUTE_BEHAVIOR", 0x01FF}, // disagrees with the define
        {"METATILE_ATTRIBUTE_LAYER_TYPE", 0xF000},
    };
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    EXPECT_EQ(find(result.candidates.front().fields, "behavior")->mask.value(), 0x00FFU); // define wins
    EXPECT_EQ(diag_.warning_tag_counts().at(metatile_attr_inference_tag), 1U);
    EXPECT_TRUE(warned_about("field 'behavior' mask define disagrees with the mask table"));
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

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 2U);
    EXPECT_EQ(find(result.candidates[0].fields, "behavior")->mask.value(), 0x00FFU);
    EXPECT_EQ(find(result.candidates[1].fields, "behavior")->mask.value(), 0x1FFU); // FRLG define wins over the table
    EXPECT_EQ(diag_.warning_tag_counts().at(metatile_attr_inference_tag), 1U);
    EXPECT_TRUE(warned_about("field 'behavior' FRLG mask define disagrees with the mask table"));
}

TEST_F(MetatileAttributeInferenceTest, AmbiguousConditionalMaskIsInvalid)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
    scan.ambiguous_defines.insert("METATILE_ATTR_BEHAVIOR_MASK");
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    EXPECT_EQ(result.status, AttributeInferenceStatus::invalid);
    EXPECT_NE(result.error_message.find("METATILE_ATTR_BEHAVIOR_MASK"), std::string::npos);
    EXPECT_NE(result.error_message.find("metatile_attribute_field_overrides"), std::string::npos);
}

// Nothing attribute-related at all means the provider should defer to the next provider.
TEST_F(MetatileAttributeInferenceTest, NothingFoundIsNotProvided)
{
    MetatileAttributeScan scan;
    scan.enum_members = {enum_member("SOME_UNRELATED_ENUM_MEMBER", 0)};

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    EXPECT_EQ(result.status, AttributeInferenceStatus::not_provided);
    EXPECT_TRUE(result.candidates.empty());
}

// A single one-byte mask yields a one-byte candidate: required_bytes follows the widest mask, not a floor of 2.
TEST_F(MetatileAttributeInferenceTest, OneByteMaskYieldsOneByteCandidate)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x0F}};
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    EXPECT_EQ(result.candidates.front().required_bytes, 1U);
    EXPECT_EQ(find(result.candidates.front().fields, "layer_type"), nullptr);
}

// The layer_type role field never gets a value provider, even when TILE_LAYER_TYPE_* enum members exist that the
// generic provider probe would otherwise latch onto: its values are managed by Porytiles.
TEST_F(MetatileAttributeInferenceTest, LayerTypeRoleFieldNeverGetsAProvider)
{
    MetatileAttributeScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_LAYER_TYPE", 1),
        enum_member("METATILE_ATTRIBUTE_COUNT", 2),
        enum_member("TILE_LAYER_TYPE_NORMAL", 0),
    };
    scan.masks_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 0x00FF}, {"METATILE_ATTRIBUTE_LAYER_TYPE", 0xF000}};
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    const auto *layer = find(result.candidates.front().fields, "layer_type");
    ASSERT_NE(layer, nullptr);
    EXPECT_EQ(layer->role, FieldRole::layer_type);
    EXPECT_FALSE(layer->provider.has_value());
}

// A layout with nothing beyond the layer-type role field is not usable: there is no per-metatile value to store, so
// inference reports not_provided rather than a degenerate candidate.
TEST_F(MetatileAttributeInferenceTest, LayerOnlyLayoutIsNotProvided)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_LAYER_MASK", 0xF000}};
    scan.behaviors_header_present = true;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_, &diag_);
    EXPECT_EQ(result.status, AttributeInferenceStatus::not_provided);
    EXPECT_TRUE(result.candidates.empty());
}

// The declaration width maps from the scanned element type of struct Tileset's metatileAttributes member: u8/u16/u32
// map to 1/2/4, anything else stays nullopt so the downstream "match the attribute size" default applies.
TEST_F(MetatileAttributeInferenceTest, DeclarationSizeMapsFromElementType)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
    scan.behaviors_header_present = true;

    scan.attributes_element_type = "u8";
    EXPECT_EQ(infer_metatile_attribute_candidates(scan, &formatter_, &diag_).declaration_size, 1U);
    scan.attributes_element_type = "u16";
    EXPECT_EQ(infer_metatile_attribute_candidates(scan, &formatter_, &diag_).declaration_size, 2U);
    scan.attributes_element_type = "u32";
    EXPECT_EQ(infer_metatile_attribute_candidates(scan, &formatter_, &diag_).declaration_size, 4U);
    scan.attributes_element_type = "u64";
    EXPECT_EQ(infer_metatile_attribute_candidates(scan, &formatter_, &diag_).declaration_size, std::nullopt);
    scan.attributes_element_type = std::nullopt;
    EXPECT_EQ(infer_metatile_attribute_candidates(scan, &formatter_, &diag_).declaration_size, std::nullopt);
}

// The declaration width is a project fact independent of the mask layout, so it survives every status: an invalid
// inference (ambiguous conditional define) and a not-provided inference both still carry it.
TEST_F(MetatileAttributeInferenceTest, DeclarationSizeSurvivesInvalidAndNotProvided)
{
    MetatileAttributeScan invalid_scan;
    invalid_scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
    invalid_scan.ambiguous_defines.insert("METATILE_ATTR_BEHAVIOR_MASK");
    invalid_scan.attributes_element_type = "u16";
    const auto invalid_result = infer_metatile_attribute_candidates(invalid_scan, &formatter_, &diag_);
    ASSERT_EQ(invalid_result.status, AttributeInferenceStatus::invalid);
    EXPECT_EQ(invalid_result.declaration_size, 2U);

    MetatileAttributeScan empty_scan;
    empty_scan.attributes_element_type = "u32";
    const auto empty_result = infer_metatile_attribute_candidates(empty_scan, &formatter_, &diag_);
    ASSERT_EQ(empty_result.status, AttributeInferenceStatus::not_provided);
    EXPECT_EQ(empty_result.declaration_size, 4U);
}

} // namespace
} // namespace porytiles
