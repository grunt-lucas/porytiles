#include "porytiles/domain/algorithms/metatile_attribute_inference.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "porytiles/utilities/text/plain_text_formatter.hpp"

namespace porytiles {
namespace {

class MetatileAttributeInferenceTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;

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

    [[nodiscard]] static const InferredFieldConflict *
    find_conflict(const MetatileAttributeCandidateSet &candidate, const std::string &field, FieldConflictKind kind)
    {
        auto it =
            std::find_if(candidate.conflicts.begin(), candidate.conflicts.end(), [&](const InferredFieldConflict &c) {
                return c.field_name == field && c.kind == kind;
            });
        return it == candidate.conflicts.end() ? nullptr : &*it;
    }
};

// Stock pokeemerald: mask defines only (behavior + layer), behavior constants present. One candidate set whose
// required width follows from the widest mask (the 0xF000 layer mask needs 2 bytes).
TEST_F(MetatileAttributeInferenceTest, EmeraldDefinesOnly)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}, {"METATILE_ATTR_LAYER_MASK", 0xF000}};
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;
    scan.header_source = "include/global.fieldmap.h";

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
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

    // Every fact settled cleanly, so nothing rides along for the reconciler to rule on.
    EXPECT_TRUE(candidate.conflicts.empty());
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
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;
    scan.header_source = "include/global.fieldmap.h";
    scan.masks_table_source = "src/fieldmap.c";

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
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

    // The numeric-suffix fields state that they have no name and no constants: nothing was inferred for them, so
    // nothing failed and they carry no provider conflict. No conflicts anywhere on the set.
    EXPECT_TRUE(candidate.conflicts.empty());
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
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;
    scan.header_source = "include/global.fieldmap.h";
    scan.masks_table_source = "src/fieldmap.c";

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
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
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;
    scan.header_source = "include/global.fieldmap.h";

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
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
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;
    scan.header_source = "include/global.fieldmap.h";

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
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
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
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
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
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
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
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
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
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
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
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
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
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
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
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

// When the behaviors header is absent, the behavior field is emitted without a provider and the candidate carries a
// conflict record. Inference does not rule on it: whether the missing provider is fatal depends on the user's
// overrides, which only the reconciler can see.
TEST_F(MetatileAttributeInferenceTest, AbsentBehaviorsHeaderIsRecordedAsConflict)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}, {"METATILE_ATTR_LAYER_MASK", 0xF000}};
    scan.behaviors_header = {BehaviorsHeaderSource::absent, "include/constants/metatile_behaviors.h"};

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    const auto &candidate = result.candidates.front();
    ASSERT_EQ(candidate.fields.size(), 2U); // behavior plus the layer_type role field
    EXPECT_EQ(candidate.fields[0].name, "behavior");
    EXPECT_FALSE(candidate.fields[0].provider.has_value());
    ASSERT_EQ(candidate.conflicts.size(), 1U);
    const auto *conflict = find_conflict(candidate, "behavior", FieldConflictKind::provider_behaviors_absent);
    ASSERT_NE(conflict, nullptr);
    EXPECT_EQ(conflict->probed, "include/constants/metatile_behaviors.h");
}

// The unreadable and no-constants header states each carry their own conflict kind, so the eventual error can say
// which one happened: "declares no MB_ name" is false of a header that never lexed.
TEST_F(MetatileAttributeInferenceTest, UnreadableAndEmptyBehaviorsHeadersCarryDistinctConflictKinds)
{
    for (const auto [source, kind] : {
             std::pair{BehaviorsHeaderSource::unreadable, FieldConflictKind::provider_behaviors_unreadable},
             std::pair{BehaviorsHeaderSource::no_constants, FieldConflictKind::provider_behaviors_no_constants},
         }) {
        MetatileAttributeScan scan;
        scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
        scan.behaviors_header = {source, "include/constants/metatile_behaviors.h"};

        const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
        ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
        ASSERT_EQ(result.candidates.size(), 1U);
        ASSERT_EQ(result.candidates.front().conflicts.size(), 1U);
        EXPECT_NE(find_conflict(result.candidates.front(), "behavior", kind), nullptr);
    }
}

// A scan that recorded no behaviors-header path still yields a conflict naming the canonical relative path, so the
// eventual diagnostic never prints an empty file name.
TEST_F(MetatileAttributeInferenceTest, BehaviorsConflictFallsBackToTheCanonicalPath)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
    scan.behaviors_header.source = BehaviorsHeaderSource::absent; // no path recorded

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    const auto *conflict =
        find_conflict(result.candidates.front(), "behavior", FieldConflictKind::provider_behaviors_absent);
    ASSERT_NE(conflict, nullptr);
    EXPECT_EQ(conflict->probed, "include/constants/metatile_behaviors.h");
}

// A named field whose TILE_<X>_ enum probe finds nothing is recorded as a conflict naming the probed prefix. This
// path was completely silent before conflicts existed.
TEST_F(MetatileAttributeInferenceTest, NamedFieldWithNoMatchingEnumIsRecordedAsConflict)
{
    MetatileAttributeScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_TERRAIN", 1),
        enum_member("METATILE_ATTRIBUTE_COUNT", 2),
        // No TILE_TERRAIN_* member anywhere.
    };
    scan.masks_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 0x00FF}, {"METATILE_ATTRIBUTE_TERRAIN", 0x0F00}};
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    const auto &candidate = result.candidates.front();
    EXPECT_FALSE(find(candidate.fields, "terrain")->provider.has_value());
    ASSERT_EQ(candidate.conflicts.size(), 1U);
    const auto *conflict = find_conflict(candidate, "terrain", FieldConflictKind::provider_no_matching_enum);
    ASSERT_NE(conflict, nullptr);
    EXPECT_EQ(conflict->probed, "TILE_TERRAIN_");
}

// A _TYPE field records the stripped retry probe, the last prefix actually looked for.
TEST_F(MetatileAttributeInferenceTest, TypeSuffixProbeConflictNamesTheRetriedPrefix)
{
    MetatileAttributeScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_ENCOUNTER_TYPE", 1),
        enum_member("METATILE_ATTRIBUTE_COUNT", 2),
    };
    scan.masks_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 0x00FF}, {"METATILE_ATTRIBUTE_ENCOUNTER_TYPE", 0x0F00}};
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    const auto *conflict =
        find_conflict(result.candidates.front(), "encounter_type", FieldConflictKind::provider_no_matching_enum);
    ASSERT_NE(conflict, nullptr);
    EXPECT_EQ(conflict->probed, "TILE_ENCOUNTER_");
}

// A shift table entry that disagrees with the mask offset is recorded as a conflict; the mask still resolves the
// field, since inference does not rule.
TEST_F(MetatileAttributeInferenceTest, ShiftTableMismatchIsRecordedAsConflict)
{
    MetatileAttributeScan scan;
    scan.enum_members = {
        enum_member("METATILE_ATTRIBUTE_BEHAVIOR", 0),
        enum_member("METATILE_ATTRIBUTE_LAYER_TYPE", 1),
        enum_member("METATILE_ATTRIBUTE_COUNT", 2),
    };
    scan.masks_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 0x00FF}, {"METATILE_ATTRIBUTE_LAYER_TYPE", 0xF000}};
    scan.shifts_array = {{"METATILE_ATTRIBUTE_BEHAVIOR", 4}}; // wrong: 0x00FF has offset 0
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    const auto &candidate = result.candidates.front();
    EXPECT_EQ(candidate.fields[0].mask.value(), 0x00FFU);
    ASSERT_EQ(candidate.conflicts.size(), 1U);
    const auto *conflict = find_conflict(candidate, "behavior", FieldConflictKind::shift_vs_mask);
    ASSERT_NE(conflict, nullptr);
    EXPECT_EQ(conflict->declared, 4U);
    EXPECT_EQ(conflict->alternate, 0U);
}

// The METATILE_ATTR_*_SHIFT defines are cross-checked against their masks the same way the shift table is. This form
// (stock pokeemerald and expansion) was checked nowhere before.
TEST_F(MetatileAttributeInferenceTest, ShiftDefineMismatchIsRecordedAsConflict)
{
    MetatileAttributeScan scan;
    scan.defines = {
        {"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF},
        {"METATILE_ATTR_BEHAVIOR_SHIFT", 0},
        {"METATILE_ATTR_LAYER_MASK", 0xF000},
        {"METATILE_ATTR_LAYER_SHIFT", 8}, // wrong: 0xF000 has offset 12
    };
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    const auto &candidate = result.candidates.front();
    ASSERT_EQ(candidate.conflicts.size(), 1U);
    // The LAYER stem normalizes to the layer_type field, matching how the mask define names it.
    const auto *conflict = find_conflict(candidate, "layer_type", FieldConflictKind::shift_vs_mask);
    ASSERT_NE(conflict, nullptr);
    EXPECT_EQ(conflict->declared, 8U);
    EXPECT_EQ(conflict->alternate, 12U);
}

// Matching shift defines are the stock pokeemerald shape and must pass silently.
TEST_F(MetatileAttributeInferenceTest, MatchingShiftDefinesRecordNoConflict)
{
    MetatileAttributeScan scan;
    scan.defines = {
        {"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF},
        {"METATILE_ATTR_BEHAVIOR_SHIFT", 0},
        {"METATILE_ATTR_LAYER_MASK", 0xF000},
        {"METATILE_ATTR_LAYER_SHIFT", 12},
    };
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    EXPECT_TRUE(result.candidates.front().conflicts.empty());
}

// In a dual layout, a bare shift define pairs with the bare mask and a _SHIFT_FRLG define pairs with the FRLG mask.
// Each conflict lands only on the set whose mask it disagrees with.
TEST_F(MetatileAttributeInferenceTest, ShiftDefinePairingFollowsTheLayoutSplit)
{
    MetatileAttributeScan scan;
    scan.defines = {
        {"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF},
        {"METATILE_ATTR_BEHAVIOR_SHIFT", 0}, // matches the bare mask
        {"METATILE_ATTR_BEHAVIOR_MASK_FRLG", 0x03FE},
        {"METATILE_ATTR_BEHAVIOR_SHIFT_FRLG", 0}, // wrong: 0x03FE has offset 1
        {"METATILE_ATTR_LAYER_MASK", 0xF000},
        {"METATILE_ATTR_LAYER_MASK_FRLG", 0x60000000},
    };
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 2U);
    EXPECT_TRUE(result.candidates[0].conflicts.empty()); // the bare set's shift matches
    ASSERT_EQ(result.candidates[1].conflicts.size(), 1U);
    const auto *conflict = find_conflict(result.candidates[1], "behavior", FieldConflictKind::shift_vs_mask);
    ASSERT_NE(conflict, nullptr);
    EXPECT_EQ(conflict->declared, 0U);
    EXPECT_EQ(conflict->alternate, 1U);
}

// Single layout: when a mask define and the mask table disagree, the define still wins the mask, and the
// disagreement is recorded as a conflict carrying both values.
TEST_F(MetatileAttributeInferenceTest, SingleLayoutDefineVsTableConflictIsRecorded)
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
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
    const auto &candidate = result.candidates.front();
    EXPECT_EQ(find(candidate.fields, "behavior")->mask.value(), 0x00FFU); // define wins
    ASSERT_EQ(candidate.conflicts.size(), 1U);
    const auto *conflict = find_conflict(candidate, "behavior", FieldConflictKind::mask_define_vs_table);
    ASSERT_NE(conflict, nullptr);
    EXPECT_EQ(conflict->declared, 0x00FFU);
    EXPECT_EQ(conflict->alternate, 0x01FFU);
}

// Dual layout: a FRLG define disagreeing with the FRLG-valued table is a conflict on the FRLG set only. The bare set
// never consults the table, so it stays clean.
TEST_F(MetatileAttributeInferenceTest, DualLayoutFrlgDefineVsTableConflictIsRecordedOnTheFrlgSet)
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
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    ASSERT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 2U);
    EXPECT_EQ(find(result.candidates[0].fields, "behavior")->mask.value(), 0x00FFU);
    EXPECT_EQ(find(result.candidates[1].fields, "behavior")->mask.value(), 0x1FFU); // FRLG define wins over the table
    EXPECT_TRUE(result.candidates[0].conflicts.empty());
    ASSERT_EQ(result.candidates[1].conflicts.size(), 1U);
    const auto *conflict = find_conflict(result.candidates[1], "behavior", FieldConflictKind::mask_define_vs_table);
    ASSERT_NE(conflict, nullptr);
    EXPECT_EQ(conflict->declared, 0x01FFU);
    EXPECT_EQ(conflict->alternate, 0x03FFU);
}

TEST_F(MetatileAttributeInferenceTest, AmbiguousConditionalMaskIsInvalid)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
    scan.ambiguous_defines.insert("METATILE_ATTR_BEHAVIOR_MASK");
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

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

// The same empty outcome means something different when a file the scan looked at could not be read: the project may
// well declare masks, they just did not reach inference. That is an error about the unreadable file, not a report
// that the project declares nothing.
TEST_F(MetatileAttributeInferenceTest, NothingFoundWithAnUnreadableSourceIsInvalid)
{
    MetatileAttributeScan scan;
    scan.unreadable_sources = {"include/global.fieldmap.h"};

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    EXPECT_EQ(result.status, AttributeInferenceStatus::invalid);
    EXPECT_TRUE(result.candidates.empty());
    EXPECT_NE(result.error_message.find("could not read"), std::string::npos) << result.error_message;
    EXPECT_NE(result.error_message.find("include/global.fieldmap.h"), std::string::npos) << result.error_message;
    EXPECT_NE(result.error_message.find("metatile_attribute_fields"), std::string::npos) << result.error_message;
}

// The unreadable-source rule covers the degenerate-layout exit too: the header parsed and declared the layer mask,
// but the mask table that would have supplied the value fields is the file that failed to read.
TEST_F(MetatileAttributeInferenceTest, LayerOnlyLayoutWithAnUnreadableSourceIsInvalid)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_LAYER_MASK", 0xF000}};
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;
    scan.unreadable_sources = {"src/fieldmap.c"};

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    EXPECT_EQ(result.status, AttributeInferenceStatus::invalid);
    EXPECT_NE(result.error_message.find("src/fieldmap.c"), std::string::npos) << result.error_message;
}

// An unreadable source is only relevant when nothing usable was inferred. When the readable files carry a complete
// layout, the unreadable one is already covered by its own warning and must not turn a valid result into an error.
TEST_F(MetatileAttributeInferenceTest, UnreadableSourceDoesNotSpoilAnOtherwiseValidLayout)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;
    scan.unreadable_sources = {"src/fieldmap.c"};

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    EXPECT_EQ(result.status, AttributeInferenceStatus::valid);
    ASSERT_EQ(result.candidates.size(), 1U);
}

// A single one-byte mask yields a one-byte candidate: required_bytes follows the widest mask, not a floor of 2.
TEST_F(MetatileAttributeInferenceTest, OneByteMaskYieldsOneByteCandidate)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x0F}};
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
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
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
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
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
    EXPECT_EQ(result.status, AttributeInferenceStatus::not_provided);
    EXPECT_TRUE(result.candidates.empty());
}

// The declaration width maps from the scanned declarator of struct Tileset's metatileAttributes member: single
// pointers to u8/u16/u32 map to 1/2/4. Anything else names no width the engine can read and stays unset, which
// reconciliation treats as fatal rather than filling in.
TEST_F(MetatileAttributeInferenceTest, DeclarationSizeMapsFromElementType)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;
    scan.declaration = {AttributeDeclarationSource::declared, "u8", 1, true};

    EXPECT_EQ(infer_metatile_attribute_candidates(scan, &formatter_).declaration.size, 1U);
    scan.declaration.element_type = "u16";
    EXPECT_EQ(infer_metatile_attribute_candidates(scan, &formatter_).declaration.size, 2U);
    scan.declaration.element_type = "u32";
    EXPECT_EQ(infer_metatile_attribute_candidates(scan, &formatter_).declaration.size, 4U);
    scan.declaration.element_type = "u64";
    EXPECT_EQ(infer_metatile_attribute_candidates(scan, &formatter_).declaration.size, std::nullopt);
}

// A width only comes from a single pointer. A value member or a pointer-to-pointer of a known type is still a
// declaration Porytiles cannot read a stride out of, so the width stays unset for reconciliation to rule on.
TEST_F(MetatileAttributeInferenceTest, DeclarationSizeRequiresASinglePointer)
{
    MetatileAttributeScan scan;
    scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
    scan.behaviors_header.source = BehaviorsHeaderSource::declared;

    scan.declaration = {AttributeDeclarationSource::declared, "u16", 0, true};
    EXPECT_EQ(infer_metatile_attribute_candidates(scan, &formatter_).declaration.size, std::nullopt);
    scan.declaration.pointer_depth = 2;
    EXPECT_EQ(infer_metatile_attribute_candidates(scan, &formatter_).declaration.size, std::nullopt);
}

// Every source with no declaration behind it leaves the width unset, and the source travels through untouched so the
// eventual error can name which of them happened.
TEST_F(MetatileAttributeInferenceTest, UndeclaredSourcesCarryThroughWithNoWidth)
{
    for (const auto source :
         {AttributeDeclarationSource::no_fieldmap_header,
          AttributeDeclarationSource::header_unreadable,
          AttributeDeclarationSource::no_tileset_struct,
          AttributeDeclarationSource::no_attributes_member}) {
        MetatileAttributeScan scan;
        scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
        scan.behaviors_header.source = BehaviorsHeaderSource::declared;
        scan.declaration.source = source;

        const auto result = infer_metatile_attribute_candidates(scan, &formatter_);
        EXPECT_EQ(result.declaration.size, std::nullopt);
        EXPECT_EQ(result.declaration.scan.source, source);
    }
}

// The declaration is a project fact independent of the mask layout, so it survives every status: an invalid
// inference (ambiguous conditional define) and a not-provided inference both still carry it.
TEST_F(MetatileAttributeInferenceTest, DeclarationSizeSurvivesInvalidAndNotProvided)
{
    MetatileAttributeScan invalid_scan;
    invalid_scan.defines = {{"METATILE_ATTR_BEHAVIOR_MASK", 0x00FF}};
    invalid_scan.ambiguous_defines.insert("METATILE_ATTR_BEHAVIOR_MASK");
    invalid_scan.declaration = {AttributeDeclarationSource::declared, "u16", 1, true};
    const auto invalid_result = infer_metatile_attribute_candidates(invalid_scan, &formatter_);
    ASSERT_EQ(invalid_result.status, AttributeInferenceStatus::invalid);
    EXPECT_EQ(invalid_result.declaration.size, 2U);

    MetatileAttributeScan empty_scan;
    empty_scan.declaration = {AttributeDeclarationSource::declared, "u32", 1, true};
    const auto empty_result = infer_metatile_attribute_candidates(empty_scan, &formatter_);
    ASSERT_EQ(empty_result.status, AttributeInferenceStatus::not_provided);
    EXPECT_EQ(empty_result.declaration.size, 4U);
}

// The rendered declaration is what the error quotes, so it has to come back exactly as the project wrote it,
// const qualifier and stars included.
TEST_F(MetatileAttributeInferenceTest, DeclarationRendersAsWritten)
{
    EXPECT_EQ(
        to_declaration_string({AttributeDeclarationSource::declared, "u16", 1, true}), "const u16 *metatileAttributes");
    EXPECT_EQ(
        to_declaration_string({AttributeDeclarationSource::declared, "MetatileAttr", 2, false}),
        "MetatileAttr **metatileAttributes");
    EXPECT_TRUE(to_declaration_string({AttributeDeclarationSource::no_tileset_struct, "", 0, false}).empty());
}

} // namespace
} // namespace porytiles
