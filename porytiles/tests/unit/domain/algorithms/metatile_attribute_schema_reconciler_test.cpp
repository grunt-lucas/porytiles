#include "porytiles/domain/algorithms/metatile_attribute_schema_reconciler.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

namespace porytiles {
namespace {

class MetatileAttributeSchemaReconcilerTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;
    BufferedUserDiagnostics diag_;

    [[nodiscard]] static MetatileAttributeFieldDefinition definition(const std::string &name, std::uint32_t mask)
    {
        MetatileAttributeFieldDefinition field_definition;
        field_definition.name = name;
        field_definition.mask = mask;
        return field_definition;
    }

    [[nodiscard]] static MetatileAttributeFieldDefinition role_definition(const std::string &name, std::uint32_t mask)
    {
        MetatileAttributeFieldDefinition field_definition;
        field_definition.name = name;
        field_definition.mask = mask;
        field_definition.role = FieldRole::layer_type;
        return field_definition;
    }

    [[nodiscard]] static MetatileAttributeCandidateSet
    candidate(std::string origin, MetatileAttributeFieldDefinitions fields, std::size_t required_bytes)
    {
        MetatileAttributeCandidateSet set;
        set.origin = std::move(origin);
        set.fields = std::move(fields);
        set.required_bytes = required_bytes;
        return set;
    }

    // The emerald shape: one 2-byte candidate (behavior 0x00FF plus the layer_type role field at 0xF000).
    [[nodiscard]] static MetatileAttributeInferenceResult emerald_inference()
    {
        MetatileAttributeInferenceResult inference;
        inference.status = AttributeInferenceStatus::valid;
        inference.candidates.push_back(candidate(
            "the METATILE_ATTR_*_MASK defines",
            {definition("behavior", 0x00FF), role_definition("layer_type", 0xF000)},
            2));
        return inference;
    }

    // The expansion shape: the bare 2-byte set and the FRLG 4-byte set.
    [[nodiscard]] static MetatileAttributeInferenceResult dual_inference()
    {
        MetatileAttributeInferenceResult inference;
        inference.status = AttributeInferenceStatus::valid;
        inference.candidates.push_back(candidate(
            "the bare METATILE_ATTR_*_MASK defines",
            {definition("behavior", 0x00FF), role_definition("layer_type", 0xF000)},
            2));
        inference.candidates.push_back(candidate(
            "the METATILE_ATTR_*_MASK_FRLG defines and the sMetatileAttrMasks table",
            {definition("behavior", 0x01FF), definition("terrain", 0x3E00), role_definition("layer_type", 0x60000000)},
            4));
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

    [[nodiscard]] std::string error_text(const ChainableResult<LoadedMetatileAttributeSchema> &result)
    {
        std::string text;
        for (const auto &err : result.chain()) {
            text += err->join(formatter_);
            text += "\n";
        }
        return text;
    }

    // The reconciler's diagnostics are single-line, but the buffer stores each one as a line vector; join so tests
    // can probe the text directly.
    [[nodiscard]] std::string warning_text(std::size_t index) const
    {
        return join_lines(diag_.warnings().at(index));
    }

    [[nodiscard]] std::string remark_text(std::size_t index) const
    {
        return join_lines(diag_.remarks().at(index));
    }

    [[nodiscard]] static std::string join_lines(const std::vector<std::string> &lines)
    {
        std::string joined;
        for (const auto &line : lines) {
            joined += line;
        }
        return joined;
    }
};

// --- The dual-layout gate. ---

TEST_F(MetatileAttributeSchemaReconcilerTest, DualLayoutWithoutPinnedSizeIsFatal)
{
    const auto result = reconcile_metatile_attribute_schema(dual_inference(), bare_inputs(), &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("more than one metatile attribute mask layout"), std::string::npos) << text;
    EXPECT_NE(text.find("fieldmap.metatile_attribute_size"), std::string::npos) << text;
    EXPECT_NE(text.find("(2 bytes)"), std::string::npos) << text;
    EXPECT_NE(text.find("(4 bytes)"), std::string::npos) << text;
}

// Explicit fields do not rescue the dual-layout width ambiguity: nothing records the read stride, and guessing it
// wrong would silently halve 4-byte attributes on expansion.
TEST_F(MetatileAttributeSchemaReconcilerTest, DualLayoutWithExplicitFieldsAndNoPinnedSizeIsStillFatal)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("custom", 0x00F0)};
    const auto result = reconcile_metatile_attribute_schema(dual_inference(), inputs, &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(error_text(result).find("more than one metatile attribute mask layout"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, DualLayoutStillFatalWithScannedDeclaration)
{
    // The scanned declaration must not soften the dual-layout ambiguity: the width is the read stride, and no
    // project file records which build flavor the user targets.
    auto inference = dual_inference();
    inference.declaration_size = 2;
    const auto result = reconcile_metatile_attribute_schema(inference, bare_inputs(), &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(error_text(result).find("more than one metatile attribute mask layout"), std::string::npos);
}

// --- Selection from inference. ---

TEST_F(MetatileAttributeSchemaReconcilerTest, UniqueCandidateSelectedAndWidthFollowsItsMasks)
{
    const auto result = reconcile_metatile_attribute_schema(emerald_inference(), bare_inputs(), &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 2U);
    EXPECT_EQ(diag_.warnings().size(), 0U);
    EXPECT_NE(result.value().size_origin.find("inferred from"), std::string::npos);
    EXPECT_NE(result.value().size_origin.find("global.fieldmap.h"), std::string::npos);
    ASSERT_EQ(result.value().schema.fields().size(), 2U);
    EXPECT_EQ(result.value().schema.fields().front().mask(), 0x00FFU);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0xF000U);
    EXPECT_EQ(result.value().fields_origin, "the METATILE_ATTR_*_MASK defines");
    // The selection remark, then the resolved-layout remark.
    ASSERT_EQ(diag_.remarks().size(), 2U);
    EXPECT_NE(remark_text(0).find("selected the metatile attribute mask layout"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, PinnedSizeSelectsTheExactWidthMatch)
{
    auto inputs = bare_inputs();
    inputs.attribute_size = 4;
    const auto result = reconcile_metatile_attribute_schema(dual_inference(), inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 4U);
    EXPECT_EQ(result.value().size_origin, "CLI option");
    EXPECT_EQ(diag_.warnings().size(), 0U);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x60000000U);
    EXPECT_NE(result.value().fields_origin.find("FRLG"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, MultipleCandidatesMatchingPinnedSizeIsFatal)
{
    MetatileAttributeInferenceResult inference;
    inference.status = AttributeInferenceStatus::valid;
    inference.candidates.push_back(
        candidate("the bare defines", {definition("behavior", 0x00FF), role_definition("layer_type", 0xF000)}, 2));
    inference.candidates.push_back(
        candidate("the FRLG defines", {definition("behavior", 0x01FF), role_definition("layer_type", 0x3000)}, 2));
    auto inputs = bare_inputs();
    inputs.attribute_size = 2;
    const auto result = reconcile_metatile_attribute_schema(inference, inputs, &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("more than one metatile attribute mask layout matching"), std::string::npos) << text;
    EXPECT_NE(text.find("metatile_attribute_fields"), std::string::npos) << text;
}

TEST_F(MetatileAttributeSchemaReconcilerTest, UniqueNarrowerFitSelectedUnderWiderKnob)
{
    MetatileAttributeInferenceResult inference;
    inference.status = AttributeInferenceStatus::valid;
    inference.candidates.push_back(candidate("the one-byte defines", {definition("behavior", 0x00FF)}, 1));
    inference.candidates.push_back(
        candidate("the FRLG defines", {definition("behavior", 0x01FF), role_definition("layer_type", 0x60000000)}, 4));
    auto inputs = bare_inputs();
    inputs.attribute_size = 2; // neither is exact; only the one-byte set fits
    const auto result = reconcile_metatile_attribute_schema(inference, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 2U);
    EXPECT_EQ(result.value().fields_origin, "the one-byte defines");
    // The knob is wider than the selected layout needs, so the wider-knob warning rides along.
    ASSERT_EQ(diag_.warnings().size(), 1U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, MultipleNarrowerFitsUnderPinnedSizeIsFatal)
{
    MetatileAttributeInferenceResult inference;
    inference.status = AttributeInferenceStatus::valid;
    inference.candidates.push_back(
        candidate("the bare defines", {definition("behavior", 0x00FF), role_definition("layer_type", 0xF000)}, 2));
    inference.candidates.push_back(
        candidate("the FRLG defines", {definition("behavior", 0x01FF), role_definition("layer_type", 0x3000)}, 2));
    auto inputs = bare_inputs();
    inputs.attribute_size = 4; // neither is exact; both fit within 4
    const auto result = reconcile_metatile_attribute_schema(inference, inputs, &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("fits within"), std::string::npos) << text;
    EXPECT_NE(text.find("metatile_attribute_fields"), std::string::npos) << text;
}

TEST_F(MetatileAttributeSchemaReconcilerTest, NoCandidateFittingPinnedSizeIsFatal)
{
    auto inputs = bare_inputs();
    inputs.attribute_size = 1; // both dual candidates need more than 1 byte
    const auto result = reconcile_metatile_attribute_schema(dual_inference(), inputs, &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("None of them fits"), std::string::npos) << text;
    EXPECT_NE(text.find("metatile_attribute_fields"), std::string::npos) << text;
    // The pinned width's config source is interpolated so the user can see where the mismatch came from.
    EXPECT_NE(text.find("CLI option"), std::string::npos) << text;
}

TEST_F(MetatileAttributeSchemaReconcilerTest, SingleCandidateWithWiderPinnedSizeSelectsAndWarns)
{
    auto inputs = bare_inputs();
    inputs.attribute_size = 4;
    const auto result = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 4U);
    EXPECT_EQ(result.value().fields_origin, "the METATILE_ATTR_*_MASK defines");
    ASSERT_EQ(diag_.warnings().size(), 1U);
    EXPECT_NE(warning_text(0).find("need only 2 bytes"), std::string::npos) << warning_text(0);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, SingleCandidateWithNarrowerPinnedSizeIsFatal)
{
    auto inputs = bare_inputs();
    inputs.attribute_size = 1; // the emerald layer mask 0xF000 cannot fit one byte
    const auto result = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("layer_type"), std::string::npos) << text;
    EXPECT_NE(text.find("metatile attribute size"), std::string::npos) << text;
}

TEST_F(MetatileAttributeSchemaReconcilerTest, InvalidInferenceIsFatalOnInferredPathEvenWithPinnedSize)
{
    auto inputs = bare_inputs();
    inputs.attribute_size = 2;
    const auto result = reconcile_metatile_attribute_schema(invalid_inference(), inputs, &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(error_text(result).find("conflicting values"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, NoCandidatesAndNoFieldsIsFatal)
{
    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, bare_inputs(), &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("No metatile attribute fields are configured"), std::string::npos) << text;
    EXPECT_NE(text.find("metatile_attribute_fields"), std::string::npos) << text;
    EXPECT_NE(text.find("METATILE_ATTR_*_MASK"), std::string::npos) << text;
}

// --- Explicit fields. ---

TEST_F(MetatileAttributeSchemaReconcilerTest, ExplicitFieldsAreTheTruthAndSkipAdoption)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("custom", 0x00F0)};
    const auto result = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    ASSERT_EQ(result.value().schema.fields().size(), 1U);
    EXPECT_EQ(result.value().schema.fields().front().name(), "custom");
    EXPECT_EQ(result.value().fields_origin, "explicit metatile_attribute_fields (porytiles/config.yaml)");
    // The width derives from the explicit masks: 0x00F0 fits one byte.
    EXPECT_EQ(result.value().attribute_bytes, 1U);
    // The explicit fields disagree with the usable inferred layout, so the mismatch warning rides along, and the
    // resolved-layout remark still summarizes what was landed on.
    ASSERT_EQ(diag_.warnings().size(), 1U);
    EXPECT_NE(warning_text(0).find("do not match"), std::string::npos);
    ASSERT_EQ(diag_.remarks().size(), 1U);
    EXPECT_NE(remark_text(0).find("resolved"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, NoMismatchWarningWhenExplicitFieldsMatchInference)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x00FF), role_definition("layer_type", 0xF000)};
    const auto result = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(diag_.warnings().size(), 0U);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0xF000U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, MismatchWarningListsFieldDifferences)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x00F0), definition("extra", 0x0F00)};
    const auto result = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    ASSERT_EQ(diag_.warnings().size(), 1U);
    const auto warning = warning_text(0);
    EXPECT_NE(warning.find("'behavior' has mask 0xF0 in the config but 0xFF in the source"), std::string::npos)
        << warning;
    EXPECT_NE(warning.find("'extra' is only in the config"), std::string::npos) << warning;
    EXPECT_NE(warning.find("'layer_type' is only in the source"), std::string::npos) << warning;
}

TEST_F(MetatileAttributeSchemaReconcilerTest, MismatchWarningFlagsRoleDifferences)
{
    // Same names and masks, but the config drops the role from layer_type's position by naming it differently is
    // covered above; here the config keeps the name but omits the role, which Schema::create would reject later.
    // Use a relocated role instead: the config puts the role on a field the source calls a plain value field.
    auto inputs = bare_inputs();
    inputs.fields = {role_definition("behavior", 0x00FF), definition("high", 0xF000)};
    const auto result = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    ASSERT_EQ(diag_.warnings().size(), 1U);
    const auto warning = warning_text(0);
    EXPECT_NE(warning.find("'behavior' carries the layer_type role only in the config"), std::string::npos) << warning;
}

TEST_F(MetatileAttributeSchemaReconcilerTest, MismatchComparisonUnderPinnedSizeUsesTheExactWidthCandidate)
{
    // With multiple inferred layouts and a pinned size, the advisory mismatch comparison picks the candidate whose
    // width matches the knob, mirroring how selection would have chosen it on the inferred path.
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x0FFF)};
    inputs.attribute_size = 2;
    const auto result = reconcile_metatile_attribute_schema(dual_inference(), inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    ASSERT_EQ(diag_.warnings().size(), 1U);
    const auto warning = warning_text(0);
    EXPECT_NE(warning.find("the bare METATILE_ATTR_*_MASK defines"), std::string::npos) << warning;
}

TEST_F(MetatileAttributeSchemaReconcilerTest, MismatchComparisonUnderPinnedSizeFallsBackToTheUniqueNarrowerFit)
{
    // No candidate matches the knob exactly, but exactly one fits within it: that candidate anchors the comparison.
    MetatileAttributeInferenceResult inference;
    inference.status = AttributeInferenceStatus::valid;
    inference.candidates.push_back(candidate("the METATILE_ATTR_*_MASK defines", {definition("behavior", 0x003F)}, 1));
    inference.candidates.push_back(candidate(
        "the sMetatileAttrMasks table",
        {definition("behavior", 0x01FF), role_definition("layer_type", 0x60000000)},
        4));
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x0FFF)};
    inputs.attribute_size = 2;
    const auto result = reconcile_metatile_attribute_schema(inference, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    ASSERT_EQ(diag_.warnings().size(), 1U);
    const auto warning = warning_text(0);
    EXPECT_NE(warning.find("the METATILE_ATTR_*_MASK defines"), std::string::npos) << warning;
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ExplicitFieldsWithoutRoleFieldDisableLayerType)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x00FF)};
    const auto result = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    // Omitting the role field means layer types are disabled; the inferred layer mask is never inherited.
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0U);
    EXPECT_EQ(result.value().schema.layer_type_field(), nullptr);
    EXPECT_NE(remark_text(diag_.remarks().size() - 1).find("layer type disabled"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ExplicitFieldsRescueInvalidInference)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x00FF)};
    const auto result = reconcile_metatile_attribute_schema(invalid_inference(), inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 1U);
    // No usable inferred layout exists, so no mismatch warning can fire.
    EXPECT_EQ(diag_.warnings().size(), 0U);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ExplicitFieldsDeriveWidthFromMasks)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("wide", 0x30000)};
    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 4U);
    EXPECT_EQ(diag_.warnings().size(), 0U);
    EXPECT_NE(
        result.value().size_origin.find("derived from the explicit metatile_attribute_fields"), std::string::npos);
}

// --- Width: knob conflicts and warnings. ---

TEST_F(MetatileAttributeSchemaReconcilerTest, MaskExceedingPinnedWidthIsFatal)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("wide", 0x30000)};
    inputs.attribute_size = 2;
    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("wide"), std::string::npos) << text;
    EXPECT_NE(text.find("metatile attribute size"), std::string::npos) << text;
    EXPECT_NE(text.find("CLI option"), std::string::npos) << text;
}

TEST_F(MetatileAttributeSchemaReconcilerTest, WiderKnobThanMasksWarns)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x00FF)};
    inputs.attribute_size = 2;
    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 2U);
    ASSERT_EQ(diag_.warnings().size(), 1U);
    const auto warning = warning_text(0);
    EXPECT_NE(warning.find("need only 1 bytes"), std::string::npos) << warning;
    EXPECT_NE(warning.find("CLI option"), std::string::npos) << warning;
}

TEST_F(MetatileAttributeSchemaReconcilerTest, WiderKnobWarningSuppressedWhenScannedDeclarationCorroborates)
{
    MetatileAttributeInferenceResult inference;
    inference.declaration_size = 2; // struct Tileset declares a u16 element, corroborating the knob
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x00FF)};
    inputs.attribute_size = 2;
    const auto result = reconcile_metatile_attribute_schema(inference, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 2U);
    EXPECT_EQ(diag_.warnings().size(), 0U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, PinnedSizeIgnoresWiderScannedDeclaration)
{
    // An explicit width is the user's truth: the scanned declaration only feeds the declaration width.
    auto inference = emerald_inference();
    inference.declaration_size = 4;
    auto inputs = bare_inputs();
    inputs.attribute_size = 2;
    const auto result = reconcile_metatile_attribute_schema(inference, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 2U);
    EXPECT_EQ(result.value().declaration_bytes, 4U);
    EXPECT_EQ(result.value().size_origin, "CLI option");
    EXPECT_EQ(diag_.warnings().size(), 0U);
}

// --- Width: the scanned declaration vs. the mask-derived width. ---

// A mask layout proves a minimum width, never the width itself, and an attribute entry is never narrower than the
// element type it is stored in. When the two disagree, only the explicit knob can say which width the project
// reads, so the conflict is fatal rather than silently resolved either way.
TEST_F(MetatileAttributeSchemaReconcilerTest, ScannedDeclarationWiderThanMaskWidthIsFatal)
{
    MetatileAttributeInferenceResult inference;
    inference.status = AttributeInferenceStatus::valid;
    inference.candidates.push_back(candidate("the METATILE_ATTR_*_MASK defines", {definition("behavior", 0x00FF)}, 1));
    inference.declaration_size = 2;
    const auto result = reconcile_metatile_attribute_schema(inference, bare_inputs(), &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("struct Tileset"), std::string::npos) << text;
    EXPECT_NE(text.find("fieldmap.metatile_attribute_size"), std::string::npos) << text;
    // The selection remark was emitted before the conflict was discovered and survives onto the error path.
    EXPECT_EQ(diag_.remarks().size(), 1U);
}

// Expansion deliberately decouples the two widths, packing 4-byte FRLG entries into a 'const u16' array. A
// declaration narrower than the mask-derived width is fine and only feeds the declaration width.
TEST_F(MetatileAttributeSchemaReconcilerTest, ScannedDeclarationNarrowerThanMaskWidthCannotNarrow)
{
    MetatileAttributeInferenceResult inference;
    inference.status = AttributeInferenceStatus::valid;
    inference.candidates.push_back(candidate(
        "the METATILE_ATTR_*_MASK_FRLG defines and the sMetatileAttrMasks table",
        {definition("behavior", 0x01FF), role_definition("layer_type", 0x60000000)},
        4));
    inference.declaration_size = 2;
    const auto result = reconcile_metatile_attribute_schema(inference, bare_inputs(), &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 4U);
    EXPECT_EQ(result.value().declaration_bytes, 2U);
    EXPECT_EQ(diag_.warnings().size(), 0U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ScannedDeclarationEqualToMaskWidthAddsNoWarning)
{
    // The stock pokeemerald shape: masks and declaration agree at 2 bytes, so nothing conflicts and nothing warns.
    auto inference = emerald_inference();
    inference.declaration_size = 2;
    const auto result = reconcile_metatile_attribute_schema(inference, bare_inputs(), &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 2U);
    EXPECT_EQ(diag_.warnings().size(), 0U);
    EXPECT_NE(result.value().size_origin.find("METATILE_ATTR_*_MASK defines"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, OverrideWideningPastScannedDeclarationResolvesTheConflict)
{
    // The width follows the merged (post-override) masks: an override that widens past the scanned declaration
    // leaves nothing in conflict, so no knob is required.
    MetatileAttributeInferenceResult inference;
    inference.status = AttributeInferenceStatus::valid;
    inference.candidates.push_back(candidate("the METATILE_ATTR_*_MASK defines", {definition("behavior", 0x00FF)}, 1));
    inference.declaration_size = 2;
    auto inputs = bare_inputs();
    inputs.overrides["behavior"] = MetatileAttributeFieldOverride{0x30000U, std::nullopt, std::nullopt, std::nullopt};
    const auto result = reconcile_metatile_attribute_schema(inference, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 4U);
    EXPECT_EQ(result.value().declaration_bytes, 2U);
    EXPECT_EQ(diag_.warnings().size(), 0U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ExplicitDeclarationKnobDoesNotWidenDerivedWidth)
{
    // The declaration-size knob controls generated C declarations only; it never feeds the width decision.
    MetatileAttributeInferenceResult inference;
    inference.status = AttributeInferenceStatus::valid;
    inference.candidates.push_back(candidate("the METATILE_ATTR_*_MASK defines", {definition("behavior", 0x00FF)}, 1));
    auto inputs = bare_inputs();
    inputs.declaration_size = 2;
    const auto result = reconcile_metatile_attribute_schema(inference, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 1U);
    EXPECT_EQ(result.value().declaration_bytes, 2U);
    EXPECT_EQ(diag_.warnings().size(), 0U);
}

// --- Declaration-width precedence. ---

TEST_F(MetatileAttributeSchemaReconcilerTest, DeclarationPrecedenceUserBeatsStructBeatsWidth)
{
    // Struct-inferred beats the width fallback. The declaration is narrower than the mask width (expansion's FRLG
    // shape), so it cannot influence the width decision.
    MetatileAttributeInferenceResult inference;
    inference.status = AttributeInferenceStatus::valid;
    inference.candidates.push_back(candidate(
        "the METATILE_ATTR_*_MASK_FRLG defines and the sMetatileAttrMasks table",
        {definition("behavior", 0x01FF), role_definition("layer_type", 0x60000000)},
        4));
    inference.declaration_size = 2;
    auto inputs = bare_inputs();
    const auto from_struct = reconcile_metatile_attribute_schema(inference, inputs, &formatter_, &diag_);
    ASSERT_TRUE(from_struct.has_value()) << error_text(from_struct);
    EXPECT_EQ(from_struct.value().attribute_bytes, 4U);
    EXPECT_EQ(from_struct.value().declaration_bytes, 2U);
    EXPECT_NE(from_struct.value().declaration_origin.find("struct Tileset"), std::string::npos);

    // The explicit knob beats the struct inference.
    inputs.declaration_size = 1;
    const auto from_knob = reconcile_metatile_attribute_schema(inference, inputs, &formatter_, &diag_);
    ASSERT_TRUE(from_knob.has_value()) << error_text(from_knob);
    EXPECT_EQ(from_knob.value().declaration_bytes, 1U);
    EXPECT_NE(from_knob.value().declaration_origin.find("explicit"), std::string::npos);

    // Neither set: the declaration width follows the resolved attribute width.
    const auto fallback = reconcile_metatile_attribute_schema(emerald_inference(), bare_inputs(), &formatter_, &diag_);
    ASSERT_TRUE(fallback.has_value()) << error_text(fallback);
    EXPECT_EQ(fallback.value().declaration_bytes, 2U);
    EXPECT_NE(fallback.value().declaration_origin.find("matches"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, DeclarationFallbackTracksMaskDerivedWidth)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("wide", 0x30000)};
    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().declaration_bytes, 4U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ExplicitDeclarationSizeNarrowerThanWidthKept)
{
    // Expansion's FRLG build shape: 4-byte attribute entries declared 'const u16'. The declaration width follows the
    // explicit knob even though it is narrower than the attribute width.
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x01FF), role_definition("layer_type", 0x60000000)};
    inputs.attribute_size = 4;
    inputs.declaration_size = 2;
    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 4U);
    EXPECT_EQ(result.value().declaration_bytes, 2U);
}

// --- The layer_type role field through the uniform field machinery. ---

TEST_F(MetatileAttributeSchemaReconcilerTest, RoleFieldMaskParticipatesInWidthDerivation)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x00FF), role_definition("layer_type", 0x60000000)};
    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 4U);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x60000000U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, RoleFieldMaskExceedingPinnedWidthIsFatal)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x00FF), role_definition("layer_type", 0x60000000)};
    inputs.attribute_size = 2;
    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("metatile attribute size"), std::string::npos) << text;
    EXPECT_NE(text.find("layer_type"), std::string::npos) << text; // names the offending mask
}

TEST_F(MetatileAttributeSchemaReconcilerTest, PinnedWidthThatCoversMasksSucceeds)
{
    // Pinned 4 bytes, and a 4-byte role mask fits exactly: no conflict, no warning. Narrow value-field masks are
    // fine (Porymap parity): unused high bits stay zero.
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x00FF), role_definition("layer_type", 0x60000000)};
    inputs.attribute_size = 4;
    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 4U);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x60000000U);
    EXPECT_EQ(diag_.warnings().size(), 0U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, LayerTypeMaskOverrideRelocatesTheRoleField)
{
    // The role field participates in the uniform override machinery: overriding its mask relocates the layer bits.
    auto inputs = bare_inputs();
    inputs.overrides["layer_type"] = MetatileAttributeFieldOverride{0x0300U, std::nullopt, std::nullopt, std::nullopt};
    const auto result = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x0300U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, RoleOverrideMovesTheRoleToAnotherField)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x00FF), definition("high", 0xF000)};
    MetatileAttributeFieldOverride role_override;
    role_override.role = std::optional<FieldRole>{FieldRole::layer_type};
    inputs.overrides["high"] = role_override;
    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0xF000U);
    ASSERT_NE(result.value().schema.layer_type_field(), nullptr);
    EXPECT_EQ(result.value().schema.layer_type_field()->name(), "high");
}

TEST_F(MetatileAttributeSchemaReconcilerTest, RoleOverrideClearsTheRole)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x00FF), role_definition("lt_bits", 0xF000)};
    MetatileAttributeFieldOverride role_override;
    role_override.role = std::optional<FieldRole>{std::nullopt}; // role: null
    inputs.overrides["lt_bits"] = role_override;
    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    // With the role cleared, lt_bits is an ordinary value field and layer types are disabled.
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0U);
    EXPECT_EQ(result.value().schema.layer_type_field(), nullptr);
    EXPECT_EQ(result.value().schema.value_fields().size(), 2U);
}

// --- Merge machinery. ---

TEST_F(MetatileAttributeSchemaReconcilerTest, OverridesMergeIntoAdoptedFields)
{
    auto inputs = bare_inputs();
    MetatileAttributeFieldOverride override_value;
    override_value.mask = 0x003F;
    inputs.overrides.emplace("behavior", override_value);
    const auto result = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().schema.fields().front().mask(), 0x003FU);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, BuildsSchemaFromMaskFields)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x00FF), definition("extra", 0x0F00)};
    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    // Exactly the configured fields: no role field is ever synthesized behind the user's back.
    EXPECT_EQ(result.value().schema.fields().size(), 2U);
    EXPECT_EQ(result.value().loaded_definitions.size(), 2U);
    EXPECT_EQ(result.value().schema.fields().front().mask(), 0x00FFU);
    EXPECT_EQ(result.value().attribute_bytes, 2U); // 0x0F00 reaches bit 12
}

TEST_F(MetatileAttributeSchemaReconcilerTest, MaskOverrideReplacesExplicitBaseline)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x00FF)};
    inputs.overrides["behavior"] = MetatileAttributeFieldOverride{0x01FFU, std::nullopt, std::nullopt, std::nullopt};
    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().schema.fields().front().mask(), 0x01FFU);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ProviderRemovalDropsProvider)
{
    auto inputs = bare_inputs();
    MetatileAttributeFieldDefinition behavior = definition("behavior", 0x00FF);
    behavior.provider = ProviderDefinition{"include/constants/metatile_behaviors.h", "MB_", {}, HeaderFormat::either};
    inputs.fields = {behavior};
    MetatileAttributeFieldOverride override_value;
    override_value.provider = ProviderDefinitionOverride{.remove = true};
    inputs.overrides["behavior"] = override_value;

    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_FALSE(result.value().schema.fields().front().has_provider());
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ProviderPartialOverrideReplacesPrefixAndSkipWholesale)
{
    auto inputs = bare_inputs();
    MetatileAttributeFieldDefinition behavior = definition("behavior", 0x00FF);
    behavior.provider = ProviderDefinition{"h.h", "OLD_", {"OLD_INVALID"}, HeaderFormat::either};
    inputs.fields = {behavior};
    MetatileAttributeFieldOverride override_value;
    ProviderDefinitionOverride provider_override;
    provider_override.prefix = "NEW_";
    provider_override.skipped = std::unordered_set<std::string>{"NEW_SKIP"};
    override_value.provider = provider_override;
    inputs.overrides["behavior"] = override_value;

    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    const auto &merged = result.value().schema.fields().front().provider_definition();
    EXPECT_EQ(merged.prefix, "NEW_");
    EXPECT_EQ(merged.header, "h.h"); // header untouched
    EXPECT_TRUE(merged.skipped.contains("NEW_SKIP"));
    EXPECT_FALSE(merged.skipped.contains("OLD_INVALID")); // wholesale replacement
}

TEST_F(MetatileAttributeSchemaReconcilerTest, DefaultOverrideReplacesBaselineDefault)
{
    auto inputs = bare_inputs();
    MetatileAttributeFieldOverride override_value;
    override_value.default_value = 7;
    inputs.overrides["behavior"] = override_value;
    const auto result = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().schema.fields().front().default_value(), 7U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ProviderOverrideReplacesHeaderAndFormat)
{
    auto inputs = bare_inputs();
    MetatileAttributeFieldDefinition behavior = definition("behavior", 0x00FF);
    behavior.provider = ProviderDefinition{"old.h", "MB_", {}, HeaderFormat::either};
    inputs.fields = {behavior};
    MetatileAttributeFieldOverride override_value;
    ProviderDefinitionOverride provider_override;
    provider_override.header = "new.h";
    provider_override.format = HeaderFormat::defines_only;
    override_value.provider = provider_override;
    inputs.overrides["behavior"] = override_value;

    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    const auto &merged = result.value().schema.fields().front().provider_definition();
    EXPECT_EQ(merged.header, "new.h");
    EXPECT_EQ(merged.prefix, "MB_"); // prefix untouched
    EXPECT_EQ(merged.format, HeaderFormat::defines_only);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ProviderOverrideOntoRawFieldLackingHeaderIsError)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("terrain", 0x3E00)};
    MetatileAttributeFieldOverride override_value;
    ProviderDefinitionOverride provider_override;
    provider_override.prefix = "TILE_TERRAIN_"; // no header supplied, and the raw field has none
    override_value.provider = provider_override;
    inputs.overrides["terrain"] = override_value;

    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(error_text(result).find("header"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, UnknownOverrideNameIsErrorListingAvailable)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x00FF), definition("terrain", 0x3E00)};
    inputs.overrides["nonexistent"] = MetatileAttributeFieldOverride{0x1U, std::nullopt, std::nullopt, std::nullopt};

    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("nonexistent"), std::string::npos);
    EXPECT_NE(text.find("behavior, terrain"), std::string::npos); // available names listed, comma-joined
}

TEST_F(MetatileAttributeSchemaReconcilerTest, DuplicateBaselineNameIsError)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x00FF), definition("behavior", 0x0F00)};
    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(error_text(result).find("more than once"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, FieldWithNoMaskIsError)
{
    auto inputs = bare_inputs();
    inputs.fields = {MetatileAttributeFieldDefinition{"behavior", std::nullopt, std::nullopt, std::nullopt}};
    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(error_text(result).find("no mask"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, FieldsFailingSchemaValidationAreFatalAndKeepDiagnostics)
{
    // The likely real-world shape of this mistake: a user declares a field named layer_type but forgets the role
    // marker. The merged definitions pass the merge checks and fail Schema::create, and anything emitted before the
    // failure (here the mismatch warning, since the role difference disagrees with inference) must survive.
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x00FF), definition("layer_type", 0xF000)};
    const auto result = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("do not form a valid layout"), std::string::npos) << text;
    EXPECT_NE(text.find("does not carry the layer_type role"), std::string::npos) << text;
    EXPECT_EQ(diag_.warnings().size(), 1U);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, OneByteMasksDeriveOneByteWidth)
{
    // No floor: a layout whose masks all fit one byte is a one-byte layout.
    auto inputs = bare_inputs();
    inputs.fields = {definition("behavior", 0x0F)};
    const auto result =
        reconcile_metatile_attribute_schema(MetatileAttributeInferenceResult{}, inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 1U);
}

// --- Diagnostics: contents and order. ---

TEST_F(MetatileAttributeSchemaReconcilerTest, WarningOrderIsMismatchThenWiderKnob)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("custom", 0x00F0)};
    inputs.attribute_size = 2; // wider than the 1 byte the mask needs
    const auto result = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    ASSERT_EQ(diag_.warnings().size(), 2U);
    EXPECT_NE(warning_text(0).find("do not match"), std::string::npos);
    EXPECT_NE(warning_text(1).find("need only"), std::string::npos);
    ASSERT_EQ(diag_.remarks().size(), 1U);
    EXPECT_NE(remark_text(0).find("resolved"), std::string::npos);
}

TEST_F(MetatileAttributeSchemaReconcilerTest, ResolvedRemarkNamesFieldsAndLayerMask)
{
    const auto result = reconcile_metatile_attribute_schema(emerald_inference(), bare_inputs(), &formatter_, &diag_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    const auto resolved = remark_text(diag_.remarks().size() - 1);
    EXPECT_NE(resolved.find("2-byte"), std::string::npos) << resolved;
    EXPECT_NE(resolved.find("behavior"), std::string::npos) << resolved;
    EXPECT_NE(resolved.find("layer type mask 0xF000"), std::string::npos) << resolved;
}

// A diagnostic emitted before a later fatal must still reach the user: here the mismatch warning is decided before
// an override error kills reconciliation.
TEST_F(MetatileAttributeSchemaReconcilerTest, DiagnosticsSurviveOntoTheErrorPath)
{
    auto inputs = bare_inputs();
    inputs.fields = {definition("custom", 0x00F0)};
    inputs.overrides["nonexistent"] = MetatileAttributeFieldOverride{0x1U, std::nullopt, std::nullopt, std::nullopt};
    const auto result = reconcile_metatile_attribute_schema(emerald_inference(), inputs, &formatter_, &diag_);
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(diag_.warnings().size(), 1U);
    EXPECT_NE(warning_text(0).find("do not match"), std::string::npos);
}

} // namespace
} // namespace porytiles
