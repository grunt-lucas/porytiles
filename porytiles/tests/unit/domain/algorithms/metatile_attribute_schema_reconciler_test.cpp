#include "porytiles/domain/algorithms/metatile_attribute_schema_reconciler.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "porytiles/utilities/text/plain_text_formatter.hpp"

namespace porytiles {
namespace {

class MetatileAttributeSchemaReconcilerTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;

    [[nodiscard]] static MetatileAttributeFieldSpec spec(const std::string &name, std::uint32_t mask)
    {
        MetatileAttributeFieldSpec field_spec;
        field_spec.name = name;
        field_spec.mask = mask;
        return field_spec;
    }

    [[nodiscard]] static MetatileAttributeCandidateSet candidate(
        std::string origin,
        MetatileAttributeFieldSpecs fields,
        std::optional<std::uint32_t> layer_mask,
        std::size_t required_bytes,
        bool synthesized = false)
    {
        MetatileAttributeCandidateSet set;
        set.origin = std::move(origin);
        set.fields = std::move(fields);
        set.layer_type_mask = layer_mask;
        set.required_bytes = required_bytes;
        set.synthesized = synthesized;
        return set;
    }

    // The emerald shape: one real 2-byte candidate (behavior 0x00FF, layer 0xF000).
    [[nodiscard]] static MetatileAttributeInferenceResult emerald_inference()
    {
        MetatileAttributeInferenceResult inference;
        inference.status = AttributeInferenceStatus::valid;
        inference.candidates.push_back(
            candidate("the METATILE_ATTR_*_MASK defines", {spec("behavior", 0x00FF)}, 0xF000, 2));
        return inference;
    }

    // The expansion shape: the bare 2-byte set and the FRLG 4-byte set.
    [[nodiscard]] static MetatileAttributeInferenceResult dual_inference()
    {
        MetatileAttributeInferenceResult inference;
        inference.status = AttributeInferenceStatus::valid;
        inference.candidates.push_back(
            candidate("the bare METATILE_ATTR_*_MASK defines", {spec("behavior", 0x00FF)}, 0xF000, 2));
        inference.candidates.push_back(candidate(
            "the METATILE_ATTR_*_MASK_FRLG defines and the sMetatileAttrMasks table",
            {spec("behavior", 0x01FF), spec("terrain", 0x3E00)},
            0x60000000,
            4));
        return inference;
    }

    // The stock behavior-only shape: one synthesized 2-byte candidate.
    [[nodiscard]] static MetatileAttributeInferenceResult synthesized_inference()
    {
        MetatileAttributeInferenceResult inference;
        inference.status = AttributeInferenceStatus::valid;
        inference.candidates.push_back(candidate(
            "the stock two-byte behavior-only layout (assumed)",
            {spec("behavior", 0x00FF)},
            std::nullopt,
            2,
            /*synthesized=*/true));
        return inference;
    }

    [[nodiscard]] static MetatileAttributeInferenceResult invalid_inference()
    {
        MetatileAttributeInferenceResult inference;
        inference.status = AttributeInferenceStatus::invalid;
        inference.error_message = "mask define has conflicting values in a conditional";
        return inference;
    }

    [[nodiscard]] static MetatileAttributeConfigInputs bare_inputs()
    {
        MetatileAttributeConfigInputs inputs;
        inputs.fields_source = "porytiles/config.yaml";
        inputs.attribute_size_source = "CLI option";
        inputs.scan_source = "include/global.fieldmap.h";
        return inputs;
    }

    [[nodiscard]] std::string error_text(const MetatileAttributeReconciliation &reconciliation)
    {
        std::string text;
        for (const auto &err : reconciliation.result.chain()) {
            text += err->join(formatter_);
            text += "\n";
        }
        return text;
    }

    [[nodiscard]] static std::size_t
    count_notes(const MetatileAttributeReconciliation &reconciliation, AttributeNoteSeverity severity)
    {
        std::size_t count = 0;
        for (const auto &note : reconciliation.notes) {
            if (note.severity == severity) {
                ++count;
            }
        }
        return count;
    }
};

// --- Step 1: width and authoritativeness. ---

TEST_F(MetatileAttributeSchemaReconcilerTest, UniqueRealCandidatePinsWidthAuthoritatively)
{
    const auto reconciliation = reconcile_metatile_attribute_schema(emerald_inference(), bare_inputs(), &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    EXPECT_EQ(reconciliation.result.value().attribute_bytes, 2U);
    // Authoritative width: no assumed-width warning rides along.
    EXPECT_EQ(count_notes(reconciliation, AttributeNoteSeverity::warning), 0U);
    EXPECT_NE(reconciliation.result.value().size_origin.find("inferred from"), std::string::npos);
    EXPECT_NE(reconciliation.result.value().size_origin.find("global.fieldmap.h"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, PinnedSizeBeatsInferredCandidateWidth)
{
    auto inputs = bare_inputs();
    inputs.attribute_size = 4;
    const auto reconciliation = reconcile_metatile_attribute_schema(dual_inference(), inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    EXPECT_EQ(reconciliation.result.value().attribute_bytes, 4U);
    EXPECT_EQ(reconciliation.result.value().size_origin, "CLI option");
    EXPECT_EQ(count_notes(reconciliation, AttributeNoteSeverity::warning), 0U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, DualLayoutWithoutPinnedSizeIsFatal)
{
    const auto reconciliation = reconcile_metatile_attribute_schema(dual_inference(), bare_inputs(), &formatter_);
    ASSERT_FALSE(reconciliation.result.has_value());
    const auto text = error_text(reconciliation);
    EXPECT_NE(text.find("more than one metatile attribute mask layout"), std::string::npos) << text;
    EXPECT_NE(text.find("fieldmap.metatile_attribute_size"), std::string::npos) << text;
    EXPECT_NE(text.find("(2 bytes)"), std::string::npos) << text;
    EXPECT_NE(text.find("(4 bytes)"), std::string::npos) << text;
}

// Trap (b): explicit fields do not rescue the dual-layout width ambiguity. The escape hatch makes selection
// advisory, not size determination; softening this would silently halve 4-byte attributes on expansion.
TEST_F(MetatileAttributeSchemaReconcilerTest, DualLayoutWithExplicitFieldsAndNoPinnedSizeIsStillFatal)
{
    auto inputs = bare_inputs();
    inputs.fields = {spec("custom", 0x00F0)};
    const auto reconciliation = reconcile_metatile_attribute_schema(dual_inference(), inputs, &formatter_);
    ASSERT_FALSE(reconciliation.result.has_value());
    EXPECT_NE(error_text(reconciliation).find("more than one metatile attribute mask layout"), std::string::npos);
}

// Trap (a): a synthesized candidate never pins the width. Behavior-only projects get the assumed 2 bytes plus the
// warning, and selection then still picks the synthesized set.
TEST_F(MetatileAttributeSchemaReconcilerTest, SynthesizedCandidateDoesNotPinWidthButIsSelected)
{
    const auto reconciliation =
        reconcile_metatile_attribute_schema(synthesized_inference(), bare_inputs(), &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    EXPECT_EQ(reconciliation.result.value().attribute_bytes, 2U);
    EXPECT_EQ(count_notes(reconciliation, AttributeNoteSeverity::warning), 1U);
    ASSERT_EQ(reconciliation.result.value().schema.fields().size(), 1U);
    EXPECT_EQ(reconciliation.result.value().schema.fields().front().name(), "behavior");
    EXPECT_NE(reconciliation.result.value().fields_origin.find("behavior-only layout"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, NoCandidatesAssumesTwoBytesWithWarning)
{
    auto inputs = bare_inputs();
    inputs.fields = {spec("behavior", 0x00FF)};
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    EXPECT_EQ(reconciliation.result.value().attribute_bytes, 2U);
    ASSERT_EQ(count_notes(reconciliation, AttributeNoteSeverity::warning), 1U);
    const auto &warning = reconciliation.notes.front();
    EXPECT_EQ(warning.severity, AttributeNoteSeverity::warning);
    EXPECT_NE(warning.text.find("assumed 2-byte"), std::string::npos) << warning.text;
    EXPECT_NE(warning.text.find("--metatile-attribute-size"), std::string::npos) << warning.text;
    EXPECT_NE(reconciliation.result.value().size_origin.find("assumed"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, AssumedWidthWidensSilentlyFromMasks)
{
    auto inputs = bare_inputs();
    inputs.fields = {spec("wide", 0x30000)};
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    // The assumed 2 bytes are only an assumption, so the mask widens the word without an error.
    EXPECT_EQ(reconciliation.result.value().attribute_bytes, 4U);
    EXPECT_EQ(count_notes(reconciliation, AttributeNoteSeverity::warning), 1U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, MaskExceedingAuthoritativeWidthIsFatal)
{
    auto inputs = bare_inputs();
    inputs.fields = {spec("wide", 0x30000)};
    inputs.attribute_size = 2;
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_FALSE(reconciliation.result.has_value());
    const auto text = error_text(reconciliation);
    EXPECT_NE(text.find("wide"), std::string::npos) << text;
    EXPECT_NE(text.find("metatile attribute size"), std::string::npos) << text;
}

// --- Step 2: selection. ---

TEST_F(MetatileAttributeSchemaReconcilerTest, UniqueMatchAdoptsInferredFieldsWithRemark)
{
    const auto reconciliation = reconcile_metatile_attribute_schema(emerald_inference(), bare_inputs(), &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    ASSERT_EQ(reconciliation.result.value().schema.fields().size(), 1U);
    EXPECT_EQ(reconciliation.result.value().schema.fields().front().mask(), 0x00FFU);
    EXPECT_EQ(reconciliation.result.value().fields_origin, "the METATILE_ATTR_*_MASK defines");
    ASSERT_EQ(reconciliation.notes.size(), 2U);
    EXPECT_EQ(reconciliation.notes[0].severity, AttributeNoteSeverity::remark);
    EXPECT_NE(reconciliation.notes[0].text.find("selected the metatile attribute mask layout"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ExplicitFieldsAreTheTruthAndSkipAdoption)
{
    auto inputs = bare_inputs();
    inputs.fields = {spec("custom", 0x00F0)};
    const auto reconciliation = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    ASSERT_EQ(reconciliation.result.value().schema.fields().size(), 1U);
    EXPECT_EQ(reconciliation.result.value().schema.fields().front().name(), "custom");
    EXPECT_EQ(
        reconciliation.result.value().fields_origin, "explicit metatile_attribute_fields (porytiles/config.yaml)");
    // No selection remark: the fields were not adopted from a candidate.
    ASSERT_EQ(reconciliation.notes.size(), 1U);
    EXPECT_NE(reconciliation.notes[0].text.find("resolved"), std::string::npos);
}

// Addendum A: under explicit fields an invalid inference is advisory, not fatal, because both inference errors name
// explicit fields as the remedy. Without a pinned size the width falls back to the warned assumption.
TEST_F(MetatileAttributeSchemaReconcilerTest, ExplicitFieldsRescueInvalidInference)
{
    auto inputs = bare_inputs();
    inputs.fields = {spec("behavior", 0x00FF)};
    inputs.attribute_size = 2;
    const auto reconciliation = reconcile_metatile_attribute_schema(invalid_inference(), inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    // No usable set: the layer mask falls back to the size-based default in Schema::create.
    EXPECT_EQ(reconciliation.result.value().schema.layer_type_mask(), 0xF000U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ExplicitFieldsWithUnmatchedWidthLeaveSelectionNull)
{
    auto inputs = bare_inputs();
    inputs.fields = {spec("custom", 0x00F0)};
    inputs.attribute_size = 4; // no candidate requires 4 bytes
    const auto reconciliation = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    // Selection stayed null, so the candidate's 0xF000 does not apply; the 4-byte default does.
    EXPECT_EQ(reconciliation.result.value().schema.layer_type_mask(), 0x60000000U);
}

// Trap (c) known cell: an ambiguous (invalid) inference on the inferred-fields path is fatal even when the size is
// pinned. The masks themselves are unusable, so selection must not proceed.
TEST_F(MetatileAttributeSchemaReconcilerTest, InvalidInferenceIsFatalOnInferredPathEvenWithPinnedSize)
{
    auto inputs = bare_inputs();
    inputs.attribute_size = 2;
    const auto reconciliation = reconcile_metatile_attribute_schema(invalid_inference(), inputs, &formatter_);
    ASSERT_FALSE(reconciliation.result.has_value());
    EXPECT_NE(error_text(reconciliation).find("conflicting values"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, NoCandidateMatchingPinnedSizeIsFatal)
{
    auto inputs = bare_inputs();
    inputs.attribute_size = 4;
    const auto reconciliation = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_);
    ASSERT_FALSE(reconciliation.result.has_value());
    const auto text = error_text(reconciliation);
    EXPECT_NE(text.find("(2 bytes)"), std::string::npos) << text;
    EXPECT_NE(text.find("metatile_attribute_fields"), std::string::npos) << text;
    // The pinned width's config source is interpolated so the user can see where the mismatch came from.
    EXPECT_NE(text.find("CLI option"), std::string::npos) << text;
}

TEST_F(MetatileAttributeSchemaReconcilerTest, MultipleCandidatesMatchingPinnedSizeIsFatal)
{
    MetatileAttributeInferenceResult inference;
    inference.status = AttributeInferenceStatus::valid;
    inference.candidates.push_back(candidate("the bare defines", {spec("behavior", 0x00FF)}, 0xF000, 2));
    inference.candidates.push_back(candidate("the FRLG defines", {spec("behavior", 0x01FF)}, 0x3000, 2));
    auto inputs = bare_inputs();
    inputs.attribute_size = 2;
    const auto reconciliation = reconcile_metatile_attribute_schema(inference, inputs, &formatter_);
    ASSERT_FALSE(reconciliation.result.has_value());
    const auto text = error_text(reconciliation);
    EXPECT_NE(text.find("more than one metatile attribute mask layout matching"), std::string::npos) << text;
    EXPECT_NE(text.find("metatile_attribute_fields"), std::string::npos) << text;
}

TEST_F(MetatileAttributeSchemaReconcilerTest, NoCandidatesAndNoFieldsIsFatal)
{
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, bare_inputs(), &formatter_);
    ASSERT_FALSE(reconciliation.result.has_value());
    EXPECT_NE(error_text(reconciliation).find("No metatile attribute fields are configured"), std::string::npos);
}

// --- Step 3: layer-type mask precedence. ---

TEST_F(MetatileAttributeSchemaReconcilerTest, ExplicitLayerMaskBeatsSelectedCandidate)
{
    auto inputs = bare_inputs();
    inputs.layer_type_mask = 0x0300;
    const auto reconciliation = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    EXPECT_EQ(reconciliation.result.value().schema.layer_type_mask(), 0x0300U);
}

// The selected candidate's layer mask applies even under explicit fields: the two knobs are unrelated, and a project
// that relocated its layer bits keeps them relocated when it declares its fields explicitly.
TEST_F(MetatileAttributeSchemaReconcilerTest, SelectedCandidateLayerMaskAppliesUnderExplicitFields)
{
    MetatileAttributeInferenceResult inference;
    inference.status = AttributeInferenceStatus::valid;
    inference.candidates.push_back(candidate("the defines", {spec("behavior", 0x00FF)}, 0x0F00, 2));
    auto inputs = bare_inputs();
    inputs.fields = {spec("behavior", 0x00FF)};
    const auto reconciliation = reconcile_metatile_attribute_schema(inference, inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    EXPECT_EQ(reconciliation.result.value().schema.layer_type_mask(), 0x0F00U);
}

// --- Step 5: declaration-width precedence. ---

TEST_F(MetatileAttributeSchemaReconcilerTest, DeclarationPrecedenceUserBeatsStructBeatsWidth)
{
    // Struct-inferred beats the width fallback.
    auto inference = emerald_inference();
    inference.declaration_size = 4;
    auto inputs = bare_inputs();
    const auto from_struct = reconcile_metatile_attribute_schema(inference, inputs, &formatter_);
    ASSERT_TRUE(from_struct.result.has_value()) << error_text(from_struct);
    EXPECT_EQ(from_struct.result.value().attribute_bytes, 2U);
    EXPECT_EQ(from_struct.result.value().declaration_bytes, 4U);
    EXPECT_NE(from_struct.result.value().declaration_origin.find("struct Tileset"), std::string::npos);

    // The explicit knob beats the struct inference.
    inputs.declaration_size = 1;
    const auto from_knob = reconcile_metatile_attribute_schema(inference, inputs, &formatter_);
    ASSERT_TRUE(from_knob.result.has_value()) << error_text(from_knob);
    EXPECT_EQ(from_knob.result.value().declaration_bytes, 1U);
    EXPECT_NE(from_knob.result.value().declaration_origin.find("explicit"), std::string::npos);

    // Neither set: the declaration width follows the resolved attribute width.
    const auto fallback = reconcile_metatile_attribute_schema(emerald_inference(), bare_inputs(), &formatter_);
    ASSERT_TRUE(fallback.result.has_value()) << error_text(fallback);
    EXPECT_EQ(fallback.result.value().declaration_bytes, 2U);
    EXPECT_NE(fallback.result.value().declaration_origin.find("matches"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, DeclarationFallbackTracksPostWideningWidth)
{
    auto inputs = bare_inputs();
    inputs.fields = {spec("wide", 0x30000)};
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    // The assumed 2 bytes widened to 4; the declaration fallback follows the widened width, not the assumption.
    EXPECT_EQ(reconciliation.result.value().declaration_bytes, 4U);
}

// --- Notes: contents, severity, and order. ---

TEST_F(MetatileAttributeSchemaReconcilerTest, NoteOrderIsWarningThenSelectionThenResolved)
{
    const auto reconciliation =
        reconcile_metatile_attribute_schema(synthesized_inference(), bare_inputs(), &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    ASSERT_EQ(reconciliation.notes.size(), 3U);
    EXPECT_EQ(reconciliation.notes[0].severity, AttributeNoteSeverity::warning);
    EXPECT_NE(reconciliation.notes[0].text.find("assumed"), std::string::npos);
    EXPECT_EQ(reconciliation.notes[1].severity, AttributeNoteSeverity::remark);
    EXPECT_NE(reconciliation.notes[1].text.find("selected"), std::string::npos);
    EXPECT_EQ(reconciliation.notes[2].severity, AttributeNoteSeverity::remark);
    EXPECT_NE(reconciliation.notes[2].text.find("resolved"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ResolvedRemarkNamesFieldsAndLayerMask)
{
    const auto reconciliation = reconcile_metatile_attribute_schema(emerald_inference(), bare_inputs(), &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    const auto &resolved_note = reconciliation.notes.back();
    EXPECT_NE(resolved_note.text.find("2-byte"), std::string::npos) << resolved_note.text;
    EXPECT_NE(resolved_note.text.find("behavior"), std::string::npos) << resolved_note.text;
    EXPECT_NE(resolved_note.text.find("layer type mask 0xF000"), std::string::npos) << resolved_note.text;
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ZeroLayerMaskReportsLayerTypeDisabled)
{
    auto inputs = bare_inputs();
    inputs.fields = {spec("behavior", 0x00FF)};
    inputs.attribute_size = 2;
    inputs.layer_type_mask = 0;
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    EXPECT_EQ(reconciliation.result.value().schema.layer_type_mask(), 0U);
    EXPECT_NE(reconciliation.notes.back().text.find("layer type disabled"), std::string::npos);
    // A disabled layer type does not widen the word.
    EXPECT_EQ(reconciliation.result.value().attribute_bytes, 2U);
}

// The assumed-width warning must survive onto the error path: it is decided before reconciliation can still fail.
TEST_F(MetatileAttributeSchemaReconcilerTest, NotesSurviveOntoTheErrorPath)
{
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, bare_inputs(), &formatter_);
    ASSERT_FALSE(reconciliation.result.has_value());
    ASSERT_EQ(count_notes(reconciliation, AttributeNoteSeverity::warning), 1U);
    EXPECT_NE(reconciliation.notes.front().text.find("assumed"), std::string::npos);
}

// Overrides flow through the merge: an override mask replaces the adopted candidate's baseline.
TEST_F(MetatileAttributeSchemaReconcilerTest, OverridesMergeIntoAdoptedFields)
{
    auto inputs = bare_inputs();
    MetatileAttributeFieldOverride override_value;
    override_value.mask = 0x003F;
    inputs.overrides.emplace("behavior", override_value);
    const auto reconciliation = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    EXPECT_EQ(reconciliation.result.value().schema.fields().front().mask(), 0x003FU);
}

// --- Merge and widen (migrated from the folded-in schema loader). ---

TEST_F(MetatileAttributeSchemaReconcilerTest, BuildsSchemaFromMaskFields)
{
    auto inputs = bare_inputs();
    inputs.fields = {spec("behavior", 0x00FF), spec("extra", 0x0F00)};
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    EXPECT_EQ(reconciliation.result.value().schema.fields().size(), 2U);
    EXPECT_EQ(reconciliation.result.value().loaded_specs.size(), 2U);
    EXPECT_EQ(reconciliation.result.value().schema.fields().front().mask(), 0x00FFU);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, MaskOverrideReplacesExplicitBaseline)
{
    auto inputs = bare_inputs();
    inputs.fields = {spec("behavior", 0x00FF)};
    inputs.overrides["behavior"] = MetatileAttributeFieldOverride{0x01FFU, std::nullopt, std::nullopt};
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    EXPECT_EQ(reconciliation.result.value().schema.fields().front().mask(), 0x01FFU);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ProviderRemovalDropsProvider)
{
    auto inputs = bare_inputs();
    MetatileAttributeFieldSpec behavior = spec("behavior", 0x00FF);
    behavior.provider = ProviderSpec{"include/constants/metatile_behaviors.h", "MB_", {}, HeaderFormat::either};
    inputs.fields = {behavior};
    MetatileAttributeFieldOverride override_value;
    override_value.provider = ProviderSpecOverride{.remove = true};
    inputs.overrides["behavior"] = override_value;

    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    EXPECT_FALSE(reconciliation.result.value().schema.fields().front().has_provider());
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ProviderPartialOverrideReplacesPrefixAndSkipWholesale)
{
    auto inputs = bare_inputs();
    MetatileAttributeFieldSpec behavior = spec("behavior", 0x00FF);
    behavior.provider = ProviderSpec{"h.h", "OLD_", {"OLD_INVALID"}, HeaderFormat::either};
    inputs.fields = {behavior};
    MetatileAttributeFieldOverride override_value;
    ProviderSpecOverride provider_override;
    provider_override.prefix = "NEW_";
    provider_override.skipped = std::unordered_set<std::string>{"NEW_SKIP"};
    override_value.provider = provider_override;
    inputs.overrides["behavior"] = override_value;

    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    const auto &merged = reconciliation.result.value().schema.fields().front().provider_spec();
    EXPECT_EQ(merged.prefix, "NEW_");
    EXPECT_EQ(merged.header, "h.h"); // header untouched
    EXPECT_TRUE(merged.skipped.contains("NEW_SKIP"));
    EXPECT_FALSE(merged.skipped.contains("OLD_INVALID")); // wholesale replacement
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ProviderOverrideOntoRawFieldLackingHeaderIsError)
{
    auto inputs = bare_inputs();
    inputs.fields = {spec("terrain", 0x3E00)};
    MetatileAttributeFieldOverride override_value;
    ProviderSpecOverride provider_override;
    provider_override.prefix = "TILE_TERRAIN_"; // no header supplied, and the raw field has none
    override_value.provider = provider_override;
    inputs.overrides["terrain"] = override_value;

    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_FALSE(reconciliation.result.has_value());
    EXPECT_NE(error_text(reconciliation).find("header"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, UnknownOverrideNameIsErrorListingAvailable)
{
    auto inputs = bare_inputs();
    inputs.fields = {spec("behavior", 0x00FF)};
    inputs.overrides["nonexistent"] = MetatileAttributeFieldOverride{0x1U, std::nullopt, std::nullopt};

    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_FALSE(reconciliation.result.has_value());
    const auto text = error_text(reconciliation);
    EXPECT_NE(text.find("nonexistent"), std::string::npos);
    EXPECT_NE(text.find("behavior"), std::string::npos); // available names listed
}

TEST_F(MetatileAttributeSchemaReconcilerTest, DuplicateBaselineNameIsError)
{
    auto inputs = bare_inputs();
    inputs.fields = {spec("behavior", 0x00FF), spec("behavior", 0x0F00)};
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_FALSE(reconciliation.result.has_value());
    EXPECT_NE(error_text(reconciliation).find("more than once"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, FieldWithNoMaskIsError)
{
    auto inputs = bare_inputs();
    inputs.fields = {MetatileAttributeFieldSpec{"behavior", std::nullopt, std::nullopt, std::nullopt}};
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_FALSE(reconciliation.result.has_value());
    EXPECT_NE(error_text(reconciliation).find("no mask"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, AssumedWidthNeverShrinksBelowTwo)
{
    // A one-byte mask under the assumed width: masks can widen the word but never prove it narrow, so the assumed 2
    // bytes are kept.
    auto inputs = bare_inputs();
    inputs.fields = {spec("behavior", 0x0F)};
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    EXPECT_EQ(reconciliation.result.value().attribute_bytes, 2U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, UnsetLayerMaskUsesSizeBasedDefault)
{
    auto inputs = bare_inputs();
    inputs.fields = {spec("behavior", 0x00FF)};
    inputs.attribute_size = 2;
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    // A 2-byte word with no explicit mask and no inferred layout falls back to the 0xF000 convention.
    EXPECT_EQ(reconciliation.result.value().schema.layer_type_mask(), 0x0000F000U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ExplicitLayerMaskOverridesConvention)
{
    auto inputs = bare_inputs();
    inputs.fields = {spec("behavior", 0x00FF)};
    inputs.attribute_size = 2;
    inputs.layer_type_mask = 0x0300; // instead of the 0xF000 convention; must be honored verbatim
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    EXPECT_EQ(reconciliation.result.value().schema.layer_type_mask(), 0x0300U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, WideExplicitLayerMaskWidensAssumedWord)
{
    // Small field masks plus the assumed (non-authoritative) 2 bytes, but a 4-byte layer mask forces widening to 4.
    // Silent widening is allowed here precisely because the width was only an assumption.
    auto inputs = bare_inputs();
    inputs.fields = {spec("behavior", 0x00FF)};
    inputs.layer_type_mask = 0x60000000;
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    EXPECT_EQ(reconciliation.result.value().attribute_bytes, 4U);
    EXPECT_EQ(reconciliation.result.value().schema.layer_type_mask(), 0x60000000U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, PinnedWidthExceededByLayerMaskIsFatal)
{
    // Pinned 2 bytes, but a 4-byte layer mask cannot fit. Widening would contradict the project's real width.
    auto inputs = bare_inputs();
    inputs.fields = {spec("behavior", 0x00FF)};
    inputs.attribute_size = 2;
    inputs.layer_type_mask = 0x60000000;
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_FALSE(reconciliation.result.has_value());
    const auto text = error_text(reconciliation);
    EXPECT_NE(text.find("metatile_attribute_size"), std::string::npos);
    EXPECT_NE(text.find("layer-type mask"), std::string::npos); // names the offending mask
}

TEST_F(MetatileAttributeSchemaReconcilerTest, PinnedWidthThatCoversMasksSucceeds)
{
    // Pinned 4 bytes, and a 4-byte layer mask fits: no widening needed, no error. Narrow field masks are fine
    // (Porymap parity): unused high bits stay zero.
    auto inputs = bare_inputs();
    inputs.fields = {spec("behavior", 0x00FF)};
    inputs.attribute_size = 4;
    inputs.layer_type_mask = 0x60000000;
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    EXPECT_EQ(reconciliation.result.value().attribute_bytes, 4U);
    EXPECT_EQ(reconciliation.result.value().schema.layer_type_mask(), 0x60000000U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ExplicitDeclarationSizeNarrowerThanWidthKept)
{
    // Expansion's FRLG build shape: 4-byte attribute entries declared 'const u16'. The declaration width follows the
    // explicit knob even though it is narrower than the attribute width.
    auto inputs = bare_inputs();
    inputs.fields = {spec("behavior", 0x01FF)};
    inputs.attribute_size = 4;
    inputs.layer_type_mask = 0x60000000;
    inputs.declaration_size = 2;
    const auto reconciliation =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_);
    ASSERT_TRUE(reconciliation.result.has_value()) << error_text(reconciliation);
    EXPECT_EQ(reconciliation.result.value().attribute_bytes, 4U);
    EXPECT_EQ(reconciliation.result.value().declaration_bytes, 2U);
}

} // namespace
} // namespace porytiles
