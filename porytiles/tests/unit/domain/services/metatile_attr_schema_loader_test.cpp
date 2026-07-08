#include "porytiles/domain/services/metatile_attr_schema_loader.hpp"

#include <algorithm>

#include <gtest/gtest.h>

#include "porytiles/utilities/text/plain_text_formatter.hpp"

namespace porytiles {
namespace {

class MetatileAttrSchemaLoaderTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;

    [[nodiscard]] std::string error_text(const ChainableResult<ResolvedTilesetAttrSchema> &result)
    {
        std::string text;
        for (const auto &err : result.chain()) {
            for (const auto &line : err->details(formatter_)) {
                text += line + "\n";
            }
        }
        return text;
    }

    [[nodiscard]] static const Field *schema_field(const Schema &schema, const std::string &name)
    {
        const auto &fields = schema.fields();
        auto it = std::find_if(fields.begin(), fields.end(), [&](const Field &f) { return f.name() == name; });
        return it == fields.end() ? nullptr : &*it;
    }
};

// The merge-and-validate rules below are shared by every resolve regardless of layout; they are exercised here through
// the primary layout, which selects each field's plain mask.

TEST_F(MetatileAttrSchemaLoaderTest, BuildsSchemaFromPrimaryMaskFields)
{
    MetatileAttrFieldSpecs fields = {
        {"behavior", 0x00FFU, std::nullopt, std::nullopt, std::nullopt},
        {"extra", 0x0F00U, std::nullopt, 0U, std::nullopt},
    };
    const auto result =
        resolve_tileset_attr_schema(fields, {}, AttrSchemaLayout::primary, 2, false, std::nullopt, &formatter_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().schema.fields().size(), 2U);
    EXPECT_EQ(result.value().resolved_specs.size(), 2U);
    EXPECT_EQ(schema_field(result.value().schema, "behavior")->mask(), 0x00FFU);
}

TEST_F(MetatileAttrSchemaLoaderTest, MaskOverrideReplacesBaseline)
{
    MetatileAttrFieldSpecs fields = {{"behavior", 0x00FFU, std::nullopt, std::nullopt, std::nullopt}};
    MetatileAttrFieldOverrides overrides;
    overrides["behavior"] = MetatileAttrFieldOverride{0x01FFU, std::nullopt, std::nullopt, std::nullopt};

    const auto result =
        resolve_tileset_attr_schema(fields, overrides, AttrSchemaLayout::primary, 2, false, std::nullopt, &formatter_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(schema_field(result.value().schema, "behavior")->mask(), 0x01FFU);
}

TEST_F(MetatileAttrSchemaLoaderTest, ProviderRemovalDropsProvider)
{
    ProviderSpec provider{"include/constants/metatile_behaviors.h", "MB_", {}, HeaderFormat::either};
    MetatileAttrFieldSpecs fields = {{"behavior", 0x00FFU, std::nullopt, std::nullopt, provider}};
    MetatileAttrFieldOverrides overrides;
    MetatileAttrFieldOverride override_value;
    override_value.provider = ProviderSpecOverride{.remove = true};
    overrides["behavior"] = override_value;

    const auto result =
        resolve_tileset_attr_schema(fields, overrides, AttrSchemaLayout::primary, 2, false, std::nullopt, &formatter_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_FALSE(schema_field(result.value().schema, "behavior")->has_provider());
}

TEST_F(MetatileAttrSchemaLoaderTest, ProviderPartialOverrideReplacesPrefixAndSkipWholesale)
{
    ProviderSpec provider{"h.h", "OLD_", {"OLD_INVALID"}, HeaderFormat::either};
    MetatileAttrFieldSpecs fields = {{"behavior", 0x00FFU, std::nullopt, std::nullopt, provider}};
    MetatileAttrFieldOverrides overrides;
    MetatileAttrFieldOverride override_value;
    ProviderSpecOverride po;
    po.prefix = "NEW_";
    po.skipped = std::unordered_set<std::string>{"NEW_SKIP"};
    override_value.provider = po;
    overrides["behavior"] = override_value;

    const auto result =
        resolve_tileset_attr_schema(fields, overrides, AttrSchemaLayout::primary, 2, false, std::nullopt, &formatter_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    const auto &merged = schema_field(result.value().schema, "behavior")->provider_spec();
    EXPECT_EQ(merged.prefix, "NEW_");
    EXPECT_EQ(merged.header, "h.h"); // header untouched
    EXPECT_TRUE(merged.skipped.contains("NEW_SKIP"));
    EXPECT_FALSE(merged.skipped.contains("OLD_INVALID")); // wholesale replacement
}

TEST_F(MetatileAttrSchemaLoaderTest, ProviderOverrideOntoRawFieldLackingHeaderIsError)
{
    MetatileAttrFieldSpecs fields = {{"terrain", 0x3E00U, std::nullopt, std::nullopt, std::nullopt}};
    MetatileAttrFieldOverrides overrides;
    MetatileAttrFieldOverride override_value;
    ProviderSpecOverride po;
    po.prefix = "TILE_TERRAIN_"; // no header supplied, and the raw field has none
    override_value.provider = po;
    overrides["terrain"] = override_value;

    const auto result =
        resolve_tileset_attr_schema(fields, overrides, AttrSchemaLayout::primary, 2, false, std::nullopt, &formatter_);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(error_text(result).find("header"), std::string::npos);
}

TEST_F(MetatileAttrSchemaLoaderTest, UnknownOverrideNameIsErrorListingAvailable)
{
    MetatileAttrFieldSpecs fields = {{"behavior", 0x00FFU, std::nullopt, std::nullopt, std::nullopt}};
    MetatileAttrFieldOverrides overrides;
    overrides["nonexistent"] = MetatileAttrFieldOverride{0x1U, std::nullopt, std::nullopt, std::nullopt};

    const auto result =
        resolve_tileset_attr_schema(fields, overrides, AttrSchemaLayout::primary, 2, false, std::nullopt, &formatter_);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("nonexistent"), std::string::npos);
    EXPECT_NE(text.find("behavior"), std::string::npos); // available names listed
}

TEST_F(MetatileAttrSchemaLoaderTest, EmptyFieldsIsError)
{
    const auto result =
        resolve_tileset_attr_schema({}, {}, AttrSchemaLayout::primary, 2, false, std::nullopt, &formatter_);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(error_text(result).find("metatile_attr_fields"), std::string::npos);
}

TEST_F(MetatileAttrSchemaLoaderTest, DuplicateBaselineNameIsError)
{
    MetatileAttrFieldSpecs fields = {
        {"behavior", 0x00FFU, std::nullopt, std::nullopt, std::nullopt},
        {"behavior", 0x0F00U, std::nullopt, std::nullopt, std::nullopt},
    };
    const auto result =
        resolve_tileset_attr_schema(fields, {}, AttrSchemaLayout::primary, 2, false, std::nullopt, &formatter_);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(error_text(result).find("more than once"), std::string::npos);
}

TEST_F(MetatileAttrSchemaLoaderTest, FieldWithNeitherMaskIsError)
{
    MetatileAttrFieldSpecs fields = {{"behavior", std::nullopt, std::nullopt, std::nullopt, std::nullopt}};
    const auto result =
        resolve_tileset_attr_schema(fields, {}, AttrSchemaLayout::primary, 2, false, std::nullopt, &formatter_);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(error_text(result).find("neither a mask"), std::string::npos);
}

// Layout selection.

namespace {

// behavior carries a primary mask and a wider frlg_mask; layer_type is FRLG-only and its frlg_mask reaches bit 16, so
// the FRLG layout cannot fit a two-byte attribute.
[[nodiscard]] MetatileAttrFieldSpecs dual_layout_fields()
{
    return {
        {"behavior", 0x00FFU, 0x01FFU, 0U, std::nullopt},
        {"terrain", 0x0F00U, std::nullopt, 0U, std::nullopt},     // primary-only
        {"layer_type", std::nullopt, 0x30000U, 0U, std::nullopt}, // frlg-only, needs > 16 bits
    };
}

} // namespace

TEST_F(MetatileAttrSchemaLoaderTest, FrlgSelectionPicksAlternateMasks)
{
    const auto result = resolve_tileset_attr_schema(
        dual_layout_fields(), {}, AttrSchemaLayout::frlg, 2, false, std::nullopt, &formatter_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    // behavior uses its frlg_mask, not its primary mask.
    ASSERT_NE(schema_field(result.value().schema, "behavior"), nullptr);
    EXPECT_EQ(schema_field(result.value().schema, "behavior")->mask(), 0x01FFU);
    // layer_type (frlg-only) is present; terrain (primary-only) is excluded.
    EXPECT_NE(schema_field(result.value().schema, "layer_type"), nullptr);
    EXPECT_EQ(schema_field(result.value().schema, "terrain"), nullptr);
}

TEST_F(MetatileAttrSchemaLoaderTest, PrimaryDropsAlternateOnlyFields)
{
    const auto result = resolve_tileset_attr_schema(
        dual_layout_fields(), {}, AttrSchemaLayout::primary, 2, false, std::nullopt, &formatter_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(schema_field(result.value().schema, "behavior")->mask(), 0x00FFU);
    EXPECT_NE(schema_field(result.value().schema, "terrain"), nullptr);
    // layer_type has no primary mask, so it is excluded from the primary layout, but it survives in the resolved specs.
    EXPECT_EQ(schema_field(result.value().schema, "layer_type"), nullptr);
    EXPECT_EQ(result.value().resolved_specs.size(), 3U);
}

TEST_F(MetatileAttrSchemaLoaderTest, FrlgWithZeroFrlgMasksErrors)
{
    // No field defines a frlg_mask, so the FRLG layout has nothing to select.
    MetatileAttrFieldSpecs fields = {
        {"behavior", 0x00FFU, std::nullopt, 0U, std::nullopt},
        {"terrain", 0x0F00U, std::nullopt, 0U, std::nullopt},
    };
    const auto result =
        resolve_tileset_attr_schema(fields, {}, AttrSchemaLayout::frlg, 2, false, std::nullopt, &formatter_);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("frlg_mask"), std::string::npos);
    EXPECT_NE(text.find("use_frlg_alternate_masks"), std::string::npos);
}

TEST_F(MetatileAttrSchemaLoaderTest, FrlgLayoutResolvesFourBytes)
{
    // The FRLG layout is read through the engine's hardcoded 'const u32 *' accessor, so its entry width is forced to
    // 4 bytes regardless of the detected width or the selected masks.
    const auto result = resolve_tileset_attr_schema(
        dual_layout_fields(), {}, AttrSchemaLayout::frlg, 2, false, std::nullopt, &formatter_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attr_bytes, 4U);
    EXPECT_EQ(result.value().schema.attr_bytes(), 4U);
}

TEST_F(MetatileAttrSchemaLoaderTest, FrlgLayoutIgnoresAuthoritativeNarrowWidth)
{
    // A real u16 declaration in metatiles.h (authoritative 2 bytes) does not constrain the FRLG layout: the
    // declaration only has to match the 'const u16 *metatileAttributes' struct field, while the engine reads
    // FRLG-layout attributes as 4-byte words. This is pokeemerald-expansion's stock shape. The declaration width
    // survives as declaration_bytes for generated INCBIN declarations.
    const auto result = resolve_tileset_attr_schema(
        dual_layout_fields(), {}, AttrSchemaLayout::frlg, 2, true, std::nullopt, &formatter_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attr_bytes, 4U);
    EXPECT_EQ(result.value().declaration_bytes, 2U);
}

TEST_F(MetatileAttrSchemaLoaderTest, FrlgLayoutForcesFourBytesForNarrowMasks)
{
    // Even when every selected frlg mask fits in 2 bytes, the FRLG layout still resolves 4: the engine's read stride
    // is fixed, so emitting 2-byte entries would corrupt the data. The unset layer mask then falls back to the 4-byte
    // size-based default, the FRLG bits-29..30 mask.
    MetatileAttrFieldSpecs fields = {{"behavior", std::nullopt, 0x01FFU, 0U, std::nullopt}};
    const auto result =
        resolve_tileset_attr_schema(fields, {}, AttrSchemaLayout::frlg, 2, false, std::nullopt, &formatter_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attr_bytes, 4U);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x60000000U);
}

TEST_F(MetatileAttrSchemaLoaderTest, DetectedLargerSizeKept)
{
    // A detected 4-byte width with small primary masks: the width is never shrunk below the detected value.
    MetatileAttrFieldSpecs fields = {{"behavior", 0x00FFU, std::nullopt, 0U, std::nullopt}};
    const auto result =
        resolve_tileset_attr_schema(fields, {}, AttrSchemaLayout::primary, 4, false, std::nullopt, &formatter_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attr_bytes, 4U);
}

TEST_F(MetatileAttrSchemaLoaderTest, OverridesMergeBeforeSelection)
{
    // A baseline field with only a primary mask; an override adds a frlg_mask, which the FRLG layout must then select.
    MetatileAttrFieldSpecs fields = {{"special", 0x0F00U, std::nullopt, 0U, std::nullopt}};
    MetatileAttrFieldOverrides overrides;
    overrides["special"] = MetatileAttrFieldOverride{std::nullopt, 0x0F00U, std::nullopt, std::nullopt};

    const auto result =
        resolve_tileset_attr_schema(fields, overrides, AttrSchemaLayout::frlg, 2, false, std::nullopt, &formatter_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    ASSERT_NE(schema_field(result.value().schema, "special"), nullptr);
    EXPECT_EQ(schema_field(result.value().schema, "special")->mask(), 0x0F00U);
    // The FRLG layout forces 4 bytes even though this mask would fit in 2 (it previously resolved 2 here).
    EXPECT_EQ(result.value().attr_bytes, 4U);
}

// Layer-type mask resolution.

TEST_F(MetatileAttrSchemaLoaderTest, UnsetLayerMaskUsesSizeBasedDefault)
{
    MetatileAttrFieldSpecs fields = {{"behavior", 0x00FFU, std::nullopt, 0U, std::nullopt}};
    const auto result =
        resolve_tileset_attr_schema(fields, {}, AttrSchemaLayout::primary, 2, false, std::nullopt, &formatter_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    // A 2-byte word with no explicit mask falls back to the 0xF000 convention.
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x0000F000U);
}

TEST_F(MetatileAttrSchemaLoaderTest, ExplicitLayerMaskOverridesConvention)
{
    MetatileAttrFieldSpecs fields = {{"behavior", 0x00FFU, std::nullopt, 0U, std::nullopt}};
    // 0x0300 instead of the 0xF000 convention; it must be honored verbatim.
    const auto result = resolve_tileset_attr_schema(
        fields, {}, AttrSchemaLayout::primary, 2, false, std::optional<std::uint32_t>{0x0300U}, &formatter_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x0300U);
}

TEST_F(MetatileAttrSchemaLoaderTest, ExplicitZeroLayerMaskDisablesLayerType)
{
    MetatileAttrFieldSpecs fields = {{"behavior", 0x00FFU, std::nullopt, 0U, std::nullopt}};
    const auto result = resolve_tileset_attr_schema(
        fields, {}, AttrSchemaLayout::primary, 2, false, std::optional<std::uint32_t>{0U}, &formatter_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0U);
    // A disabled layer type does not widen the word.
    EXPECT_EQ(result.value().attr_bytes, 2U);
}

TEST_F(MetatileAttrSchemaLoaderTest, WideExplicitLayerMaskWidensWord)
{
    // Small field masks (fit in 1 byte) plus a guessed (non-authoritative) 2-byte width, but a 4-byte layer mask forces
    // widening to 4. Silent widening is allowed here precisely because the width was only a default, not a declaration.
    MetatileAttrFieldSpecs fields = {{"behavior", 0x00FFU, std::nullopt, 0U, std::nullopt}};
    const auto result = resolve_tileset_attr_schema(
        fields, {}, AttrSchemaLayout::primary, 2, false, std::optional<std::uint32_t>{0x60000000U}, &formatter_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attr_bytes, 4U);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x60000000U);
}

// Authoritative width, primary layout only: a real metatiles.h declaration pins the engine-fixed width, because for
// the primary layout the declared type IS the read stride. A mask that needs a wider word is a misconfiguration (e.g.
// an FRLG layer mask pasted onto an emerald-width project), not evidence of a hidden width, so it is a hard error
// rather than a silent widen. The frlg layout is exempt: its stride is the engine's hardcoded u32 accessor.

TEST_F(MetatileAttrSchemaLoaderTest, AuthoritativeWidthExceededByLayerMaskIsError)
{
    // Declared 2 bytes (authoritative), but a 4-byte layer mask cannot fit. Widening would contradict the declaration.
    MetatileAttrFieldSpecs fields = {{"behavior", 0x00FFU, std::nullopt, 0U, std::nullopt}};
    const auto result = resolve_tileset_attr_schema(
        fields, {}, AttrSchemaLayout::primary, 2, true, std::optional<std::uint32_t>{0x60000000U}, &formatter_);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("metatiles.h"), std::string::npos);
    EXPECT_NE(text.find("layer-type mask"), std::string::npos); // names the offending mask
}

TEST_F(MetatileAttrSchemaLoaderTest, AuthoritativeWidthExceededByFieldMaskIsError)
{
    // Declared 2 bytes (authoritative), but a field mask reaches bit 20, needing a 4-byte word.
    MetatileAttrFieldSpecs fields = {{"wide", 0x80000U, std::nullopt, 0U, std::nullopt}};
    const auto result =
        resolve_tileset_attr_schema(fields, {}, AttrSchemaLayout::primary, 2, true, std::nullopt, &formatter_);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("metatiles.h"), std::string::npos);
    EXPECT_NE(text.find("wide"), std::string::npos); // names the offending field
}

TEST_F(MetatileAttrSchemaLoaderTest, AuthoritativeWidthThatCoversMasksSucceeds)
{
    // Declared 4 bytes (authoritative), and a 4-byte layer mask fits: no widening needed, no error.
    MetatileAttrFieldSpecs fields = {{"behavior", 0x00FFU, std::nullopt, 0U, std::nullopt}};
    const auto result = resolve_tileset_attr_schema(
        fields, {}, AttrSchemaLayout::primary, 4, true, std::optional<std::uint32_t>{0x60000000U}, &formatter_);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attr_bytes, 4U);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x60000000U);
}

TEST_F(MetatileAttrSchemaLoaderTest, DeclarationBytesTracksAuthoritativeWidth)
{
    MetatileAttrFieldSpecs fields = {{"behavior", 0x00FFU, std::nullopt, 0U, std::nullopt}};

    // An authoritative declared width is the declaration width (primary layout: also the resolved width).
    const auto authoritative =
        resolve_tileset_attr_schema(fields, {}, AttrSchemaLayout::primary, 4, true, std::nullopt, &formatter_);
    ASSERT_TRUE(authoritative.has_value()) << error_text(authoritative);
    EXPECT_EQ(authoritative.value().declaration_bytes, 4U);
    EXPECT_EQ(authoritative.value().attr_bytes, 4U);

    // With no declaration to follow, the declaration width matches whatever the schema resolved.
    const auto guessed = resolve_tileset_attr_schema(
        fields, {}, AttrSchemaLayout::primary, 2, false, std::optional<std::uint32_t>{0x60000000U}, &formatter_);
    ASSERT_TRUE(guessed.has_value()) << error_text(guessed);
    EXPECT_EQ(guessed.value().attr_bytes, 4U);
    EXPECT_EQ(guessed.value().declaration_bytes, 4U);
}

} // namespace
} // namespace porytiles
