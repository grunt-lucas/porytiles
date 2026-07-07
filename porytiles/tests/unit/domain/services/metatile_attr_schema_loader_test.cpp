#include "porytiles/domain/services/metatile_attr_schema_loader.hpp"

#include <algorithm>

#include <gtest/gtest.h>

#include "porytiles/utilities/text/plain_text_formatter.hpp"

namespace porytiles {
namespace {

class MetatileAttrSchemaLoaderTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;

    [[nodiscard]] std::string all_error_text(const ChainableResult<LoadedAttrSchema> &result)
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

TEST_F(MetatileAttrSchemaLoaderTest, BuildsSchemaFromPrimaryMaskFields)
{
    MetatileAttrFieldSpecs fields = {
        {"behavior", 0x00FFU, std::nullopt, std::nullopt, std::nullopt},
        {"layer", 0xF000U, std::nullopt, 0U, std::nullopt},
    };
    const auto result = load_metatile_attr_schema(fields, {}, 2, &formatter_);
    ASSERT_TRUE(result.has_value()) << all_error_text(result);
    EXPECT_EQ(result.value().schema.fields().size(), 2U);
    EXPECT_EQ(result.value().resolved_specs.size(), 2U);
    EXPECT_EQ(schema_field(result.value().schema, "behavior")->mask(), 0x00FFU);
}

TEST_F(MetatileAttrSchemaLoaderTest, MaskOverrideReplacesBaseline)
{
    MetatileAttrFieldSpecs fields = {{"behavior", 0x00FFU, std::nullopt, std::nullopt, std::nullopt}};
    MetatileAttrFieldOverrides overrides;
    overrides["behavior"] = MetatileAttrFieldOverride{0x01FFU, std::nullopt, std::nullopt, std::nullopt};

    const auto result = load_metatile_attr_schema(fields, overrides, 2, &formatter_);
    ASSERT_TRUE(result.has_value()) << all_error_text(result);
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

    const auto result = load_metatile_attr_schema(fields, overrides, 2, &formatter_);
    ASSERT_TRUE(result.has_value()) << all_error_text(result);
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

    const auto result = load_metatile_attr_schema(fields, overrides, 2, &formatter_);
    ASSERT_TRUE(result.has_value()) << all_error_text(result);
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

    const auto result = load_metatile_attr_schema(fields, overrides, 4, &formatter_);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(all_error_text(result).find("header"), std::string::npos);
}

TEST_F(MetatileAttrSchemaLoaderTest, UnknownOverrideNameIsErrorListingAvailable)
{
    MetatileAttrFieldSpecs fields = {{"behavior", 0x00FFU, std::nullopt, std::nullopt, std::nullopt}};
    MetatileAttrFieldOverrides overrides;
    overrides["nonexistent"] = MetatileAttrFieldOverride{0x1U, std::nullopt, std::nullopt, std::nullopt};

    const auto result = load_metatile_attr_schema(fields, overrides, 2, &formatter_);
    ASSERT_FALSE(result.has_value());
    const auto text = all_error_text(result);
    EXPECT_NE(text.find("nonexistent"), std::string::npos);
    EXPECT_NE(text.find("behavior"), std::string::npos); // available names listed
}

TEST_F(MetatileAttrSchemaLoaderTest, EmptyFieldsIsError)
{
    const auto result = load_metatile_attr_schema({}, {}, 2, &formatter_);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(all_error_text(result).find("metatile_attr_fields"), std::string::npos);
}

TEST_F(MetatileAttrSchemaLoaderTest, DuplicateBaselineNameIsError)
{
    MetatileAttrFieldSpecs fields = {
        {"behavior", 0x00FFU, std::nullopt, std::nullopt, std::nullopt},
        {"behavior", 0x0F00U, std::nullopt, std::nullopt, std::nullopt},
    };
    const auto result = load_metatile_attr_schema(fields, {}, 2, &formatter_);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(all_error_text(result).find("more than once"), std::string::npos);
}

TEST_F(MetatileAttrSchemaLoaderTest, AlternateOnlyFieldExcludedFromSchemaButKeptInSpecs)
{
    MetatileAttrFieldSpecs fields = {
        {"behavior", 0x00FFU, 0x1FFU, std::nullopt, std::nullopt},
        {"terrain", std::nullopt, 0x3E00U, std::nullopt, std::nullopt}, // alternate-only
    };
    const auto result = load_metatile_attr_schema(fields, {}, 2, &formatter_);
    ASSERT_TRUE(result.has_value()) << all_error_text(result);
    // Only behavior has a primary mask, so the schema has one field.
    EXPECT_EQ(result.value().schema.fields().size(), 1U);
    EXPECT_EQ(schema_field(result.value().schema, "terrain"), nullptr);
    // But both fields survive in the resolved specs.
    EXPECT_EQ(result.value().resolved_specs.size(), 2U);
}

TEST_F(MetatileAttrSchemaLoaderTest, FieldWithNeitherMaskIsError)
{
    MetatileAttrFieldSpecs fields = {{"behavior", std::nullopt, std::nullopt, std::nullopt, std::nullopt}};
    const auto result = load_metatile_attr_schema(fields, {}, 2, &formatter_);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(all_error_text(result).find("neither a mask"), std::string::npos);
}

TEST_F(MetatileAttrSchemaLoaderTest, AttrSizeViolationSurfaces)
{
    // A mask occupying bits 16-17 cannot fit a 2-byte attribute; Schema::create must reject it.
    MetatileAttrFieldSpecs fields = {{"behavior", 0x30000U, std::nullopt, std::nullopt, std::nullopt}};
    const auto result = load_metatile_attr_schema(fields, {}, 2, &formatter_);
    ASSERT_FALSE(result.has_value());
    EXPECT_FALSE(all_error_text(result).empty());
}

// Helpers for resolve_tileset_attr_schema tests.

namespace {

[[nodiscard]] std::string
resolve_error_text(const ChainableResult<ResolvedTilesetAttrSchema> &result, const PlainTextFormatter &formatter)
{
    std::string text;
    for (const auto &err : result.chain()) {
        for (const auto &line : err->details(formatter)) {
            text += line + "\n";
        }
    }
    return text;
}

[[nodiscard]] const Field *find_field(const Schema &schema, const std::string &name)
{
    const auto &fields = schema.fields();
    auto it = std::find_if(fields.begin(), fields.end(), [&](const Field &f) { return f.name() == name; });
    return it == fields.end() ? nullptr : &*it;
}

// behavior carries a primary mask and a wider frlg_mask; layer_type is FRLG-only and its frlg_mask reaches bit 16, so
// the FRLG layout cannot fit a two-byte attribute.
[[nodiscard]] MetatileAttrFieldSpecs dual_layout_fields()
{
    return {
        {"behavior", 0x00FFU, 0x01FFU, 0U, std::nullopt},
        {"terrain", 0x3F00U, std::nullopt, 0U, std::nullopt},     // primary-only
        {"layer_type", std::nullopt, 0x30000U, 0U, std::nullopt}, // frlg-only, needs > 16 bits
    };
}

} // namespace

TEST_F(MetatileAttrSchemaLoaderTest, FrlgSelectionPicksAlternateMasks)
{
    const auto result =
        resolve_tileset_attr_schema(dual_layout_fields(), {}, AttrSchemaLayout::frlg, 2, false, &formatter_);
    ASSERT_TRUE(result.has_value()) << resolve_error_text(result, formatter_);
    // behavior uses its frlg_mask, not its primary mask.
    ASSERT_NE(find_field(result.value().schema, "behavior"), nullptr);
    EXPECT_EQ(find_field(result.value().schema, "behavior")->mask(), 0x01FFU);
    // layer_type (frlg-only) is present; terrain (primary-only) is excluded.
    EXPECT_NE(find_field(result.value().schema, "layer_type"), nullptr);
    EXPECT_EQ(find_field(result.value().schema, "terrain"), nullptr);
}

TEST_F(MetatileAttrSchemaLoaderTest, PrimaryDropsAlternateOnlyFields)
{
    const auto result =
        resolve_tileset_attr_schema(dual_layout_fields(), {}, AttrSchemaLayout::primary, 2, false, &formatter_);
    ASSERT_TRUE(result.has_value()) << resolve_error_text(result, formatter_);
    EXPECT_EQ(find_field(result.value().schema, "behavior")->mask(), 0x00FFU);
    EXPECT_NE(find_field(result.value().schema, "terrain"), nullptr);
    // layer_type has no primary mask, so it is excluded from the primary layout.
    EXPECT_EQ(find_field(result.value().schema, "layer_type"), nullptr);
}

TEST_F(MetatileAttrSchemaLoaderTest, FrlgWithZeroFrlgMasksErrors)
{
    // No field defines a frlg_mask, so the FRLG layout has nothing to select.
    MetatileAttrFieldSpecs fields = {
        {"behavior", 0x00FFU, std::nullopt, 0U, std::nullopt},
        {"terrain", 0x3F00U, std::nullopt, 0U, std::nullopt},
    };
    const auto result = resolve_tileset_attr_schema(fields, {}, AttrSchemaLayout::frlg, 2, false, &formatter_);
    ASSERT_FALSE(result.has_value());
    const auto text = resolve_error_text(result, formatter_);
    EXPECT_NE(text.find("frlg_mask"), std::string::npos);
    EXPECT_NE(text.find("use_frlg_alternate_masks"), std::string::npos);
}

TEST_F(MetatileAttrSchemaLoaderTest, AttrSizeWidensSilentlyForFrlg)
{
    // Configured 2 bytes, not explicit. The FRLG layer_type mask reaches bit 16, so the schema must widen to 4.
    const auto result =
        resolve_tileset_attr_schema(dual_layout_fields(), {}, AttrSchemaLayout::frlg, 2, false, &formatter_);
    ASSERT_TRUE(result.has_value()) << resolve_error_text(result, formatter_);
    EXPECT_EQ(result.value().attr_bytes, 4U);
    EXPECT_EQ(result.value().schema.attr_bytes(), 4U);
}

TEST_F(MetatileAttrSchemaLoaderTest, ExplicitTooSmallSurfacesSchemaError)
{
    // Explicit 2 bytes wins even though it is too small for the FRLG masks; Schema::create rejects the layout.
    const auto result =
        resolve_tileset_attr_schema(dual_layout_fields(), {}, AttrSchemaLayout::frlg, 2, true, &formatter_);
    ASSERT_FALSE(result.has_value());
    EXPECT_FALSE(resolve_error_text(result, formatter_).empty());
}

TEST_F(MetatileAttrSchemaLoaderTest, ExplicitLargerSizeKept)
{
    // Explicit 4 bytes with small primary masks: the size is kept, not shrunk to the detected 2.
    MetatileAttrFieldSpecs fields = {{"behavior", 0x00FFU, std::nullopt, 0U, std::nullopt}};
    const auto result = resolve_tileset_attr_schema(fields, {}, AttrSchemaLayout::primary, 4, true, &formatter_);
    ASSERT_TRUE(result.has_value()) << resolve_error_text(result, formatter_);
    EXPECT_EQ(result.value().attr_bytes, 4U);
}

TEST_F(MetatileAttrSchemaLoaderTest, OverridesMergeBeforeSelection)
{
    // A baseline field with only a primary mask; an override adds a frlg_mask, which the FRLG layout must then select.
    MetatileAttrFieldSpecs fields = {{"special", 0x0F00U, std::nullopt, 0U, std::nullopt}};
    MetatileAttrFieldOverrides overrides;
    overrides["special"] = MetatileAttrFieldOverride{std::nullopt, 0x0F00U, std::nullopt, std::nullopt};

    const auto result = resolve_tileset_attr_schema(fields, overrides, AttrSchemaLayout::frlg, 2, false, &formatter_);
    ASSERT_TRUE(result.has_value()) << resolve_error_text(result, formatter_);
    ASSERT_NE(find_field(result.value().schema, "special"), nullptr);
    EXPECT_EQ(find_field(result.value().schema, "special")->mask(), 0x0F00U);
}

} // namespace
} // namespace porytiles
