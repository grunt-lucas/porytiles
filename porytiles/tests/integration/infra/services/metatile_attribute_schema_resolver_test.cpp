#include "porytiles/infra/services/metatile_attribute_schema_resolver.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "porytiles/infra/cli/cli_option_storage.hpp"
#include "porytiles/infra/config/cli_option_provider.hpp"
#include "porytiles/infra/config/default_provider.hpp"
#include "porytiles/infra/config/lazy_layered_config.hpp"
#include "porytiles/infra/config/yaml_file_provider.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

namespace porytiles {
namespace {

// An explicit single-field config used where the tests need fields without header inference.
constexpr auto explicit_fields_yaml = R"(
fieldmap:
  metatile_attribute_fields:
    - name: behavior
      mask: 0x00FF
)";

// Stock pokeemerald's mask defines: one candidate set, behavior plus the 0xF000 layer mask, 2 bytes.
constexpr auto emerald_defines = R"(
#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF
#define METATILE_ATTR_LAYER_MASK    0xF000
)";

// Stock pokeemerald-expansion's dual defines: the bare set (2 bytes) and the FRLG set (4 bytes).
constexpr auto dual_defines = R"(
#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF
#define METATILE_ATTR_LAYER_MASK    0xF000
#define METATILE_ATTR_BEHAVIOR_MASK_FRLG 0x01FF
#define METATILE_ATTR_LAYER_MASK_FRLG    0x60000000
)";

// Like emerald_defines, but with the layer-type bits moved off the vanilla position. Every stock decomp
// declares the vanilla mask for its width (0xF000 at two bytes, 0x60000000 at four), so a relocated mask is
// the shape that proves the resolved position really came from the project's source. 0x0F00 is still
// contiguous and still fits two bytes, so only its provenance differs.
constexpr auto relocated_layer_defines = R"(
#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF
#define METATILE_ATTR_LAYER_MASK    0x0F00
)";

constexpr auto test_tileset_name = "gTileset_Test";

class MetatileAttributeSchemaResolverTest : public ::testing::Test {
  protected:
    std::filesystem::path project_root_;
    PlainTextFormatter formatter_;
    BufferedUserDiagnostics diag_;

    void SetUp() override
    {
        // YamlFileProvider caches parsed files by path process-wide, so each test needs a distinct root.
        const auto *info = ::testing::UnitTest::GetInstance()->current_test_info();
        project_root_ = std::filesystem::temp_directory_path() /
                        (std::string{"porytiles_resolver_test_"} + info->test_suite_name() + "_" + info->name());
        std::filesystem::remove_all(project_root_);
        std::filesystem::create_directories(project_root_ / "porytiles");
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove_all(project_root_, ec);
    }

    void write_config(const std::string &yaml)
    {
        std::ofstream out{project_root_ / "porytiles" / "config.yaml"};
        out << yaml;
    }

    void write_fieldmap_header(const std::string &content)
    {
        std::filesystem::create_directories(project_root_ / "include");
        std::ofstream out{project_root_ / "include" / "global.fieldmap.h"};
        out << content;
    }

    void write_fieldmap_source(const std::string &content)
    {
        std::filesystem::create_directories(project_root_ / "src");
        std::ofstream out{project_root_ / "src" / "fieldmap.c"};
        out << content;
    }

    [[nodiscard]] ChainableResult<LoadedMetatileAttributeSchema> resolve(const std::string &tileset_name)
    {
        return resolve_impl(project_root_, tileset_name, nullptr);
    }

    // Same as resolve() but with a CliOptionProvider at the head of the chain, mirroring the real command setup.
    [[nodiscard]] ChainableResult<LoadedMetatileAttributeSchema>
    resolve_with_cli(const std::string &tileset_name, const CliOptionStorage &storage)
    {
        return resolve_impl(project_root_, tileset_name, &storage);
    }

    // Resolve against an arbitrary root (the testbed acceptance tests point this at the decomp checkouts).
    [[nodiscard]] ChainableResult<LoadedMetatileAttributeSchema>
    resolve_impl(const std::filesystem::path &root, const std::string &tileset_name, const CliOptionStorage *storage)
    {
        std::vector<std::unique_ptr<ConfigProvider>> providers;
        if (storage != nullptr) {
            providers.push_back(std::make_unique<CliOptionProvider>(*storage));
        }
        providers.push_back(std::make_unique<YamlFileProvider>(&formatter_, &diag_, root));
        providers.push_back(std::make_unique<DefaultProvider>());
        LazyLayeredConfig config{&formatter_, std::move(providers)};

        MetatileAttributeSchemaResolver resolver{root, &config, &formatter_, &diag_};
        return resolver.resolve(tileset_name);
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

    [[nodiscard]] static const Field *schema_field(const Schema &schema, const std::string &name)
    {
        for (const Field &field : schema.fields()) {
            if (field.name() == name) {
                return &field;
            }
        }
        return nullptr;
    }
};

// --- Size inference and mask-set selection. ---

TEST_F(MetatileAttributeSchemaResolverTest, SizeInferredFromSingleCandidateSet)
{
    // No explicit config at all: the single candidate set is selected and its masks answer the size, so no
    // warnings ride along.
    write_fieldmap_header(emerald_defines);

    const auto result = resolve(test_tileset_name);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 2U);
    EXPECT_EQ(result.value().declaration_bytes, 2U);
    ASSERT_NE(schema_field(result.value().schema, "behavior"), nullptr);
    EXPECT_EQ(schema_field(result.value().schema, "behavior")->mask(), 0x00FFU);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0xF000U);
    EXPECT_FALSE(diag_.warning_tag_counts().contains(metatile_attr_schema_tag));
}

TEST_F(MetatileAttributeSchemaResolverTest, EnumOnlyProjectWithNoMasksIsFatal)
{
    // A project declaring the attribute enum but no masks anywhere: there is nothing to infer a layout from, and
    // the old synthesized behavior-only completion is gone, so this is a fatal, actionable error.
    write_fieldmap_header(R"(
enum
{
    METATILE_ATTRIBUTE_BEHAVIOR,
    METATILE_ATTRIBUTE_LAYER_TYPE,
    METATILE_ATTRIBUTE_COUNT,
};
)");

    const auto result = resolve(test_tileset_name);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("behavior"), std::string::npos) << text;
    EXPECT_NE(text.find("METATILE_ATTR_BEHAVIOR_MASK"), std::string::npos) << text;
    EXPECT_NE(text.find("metatile_attribute_fields"), std::string::npos) << text;
}

TEST_F(MetatileAttributeSchemaResolverTest, UnparseableHeaderIsFatalAboutTheFileNotTheMasks)
{
    // The header declares masks but cannot be scanned (unterminated block comment). Telling the user Porytiles
    // "found no mask layout" and to "make sure those masks exist" would send them looking for masks that are right
    // there in the file, so the fatal has to be about the file it could not read.
    write_fieldmap_header(R"(
#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF /* unterminated block comment
#define METATILE_ATTR_LAYER_MASK    0xF000
)");

    const auto result = resolve(test_tileset_name);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("could not read"), std::string::npos) << text;
    EXPECT_NE(text.find("global.fieldmap.h"), std::string::npos) << text;
    EXPECT_NE(text.find("metatile_attribute_fields"), std::string::npos) << text;
    EXPECT_EQ(text.find("Make sure those masks exist"), std::string::npos) << text;
}

TEST_F(MetatileAttributeSchemaResolverTest, UnparseableHeaderStillDefersToExplicitFields)
{
    // A user who declared the layout does not need the header at all, so an unreadable one stays a warning. The
    // fatal above is specifically about having nothing to fall back on.
    write_config(explicit_fields_yaml);
    write_fieldmap_header(R"(
#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF /* unterminated block comment
)");

    const auto result = resolve(test_tileset_name);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    ASSERT_NE(schema_field(result.value().schema, "behavior"), nullptr);
    EXPECT_EQ(schema_field(result.value().schema, "behavior")->mask(), 0x00FFU);
}

TEST_F(MetatileAttributeSchemaResolverTest, MultipleCandidateSetsRequireExplicitSize)
{
    // Two mask layouts and nothing choosing between them: fatal, naming the explicit size knob.
    write_fieldmap_header(dual_defines);

    const auto result = resolve(test_tileset_name);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("more than one metatile attribute mask layout"), std::string::npos) << text;
    EXPECT_NE(text.find("fieldmap.metatile_attribute_size"), std::string::npos) << text;
}

TEST_F(MetatileAttributeSchemaResolverTest, SelectionPicksSetMatchingSize)
{
    // An explicit size rescues the dual-layout case and selects the set whose required width matches: 2 bytes picks
    // the bare defines, 4 bytes picks the FRLG defines.
    write_fieldmap_header(dual_defines);
    write_config("fieldmap:\n  metatile_attribute_size: 2\n");

    const auto two = resolve(test_tileset_name);
    ASSERT_TRUE(two.has_value()) << error_text(two);
    EXPECT_EQ(two.value().attribute_bytes, 2U);
    ASSERT_NE(schema_field(two.value().schema, "behavior"), nullptr);
    EXPECT_EQ(schema_field(two.value().schema, "behavior")->mask(), 0x00FFU);
    EXPECT_EQ(two.value().schema.layer_type_mask(), 0xF000U);

    CliOptionStorage storage;
    storage.metatile_attribute_size = "4";
    const auto four = resolve_with_cli(test_tileset_name, storage);
    ASSERT_TRUE(four.has_value()) << error_text(four);
    EXPECT_EQ(four.value().attribute_bytes, 4U);
    EXPECT_EQ(schema_field(four.value().schema, "behavior")->mask(), 0x01FFU);
    EXPECT_EQ(four.value().schema.layer_type_mask(), 0x60000000U);
}

TEST_F(MetatileAttributeSchemaResolverTest, CandidateLayerMaskSelectedBySize)
{
    // The selected set's structural layer mask follows the selection; nothing explicit is configured.
    write_fieldmap_header(dual_defines);
    CliOptionStorage storage;
    storage.metatile_attribute_size = "4";

    const auto result = resolve_with_cli(test_tileset_name, storage);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x60000000U);
}

TEST_F(MetatileAttributeSchemaResolverTest, SingleCandidateUnderWiderKnobSelectsAndWarns)
{
    // One 2-byte candidate under an explicit size of 4: the layout still selects (there is nothing else it could
    // be), and the knob being wider than anything the project declares draws a warning.
    write_fieldmap_header(emerald_defines);
    write_config("fieldmap:\n  metatile_attribute_size: 4\n");

    const auto result = resolve(test_tileset_name);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 4U);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0xF000U);
    EXPECT_TRUE(diag_.warning_tag_counts().contains(metatile_attr_schema_tag));
}

TEST_F(MetatileAttributeSchemaResolverTest, MultipleCandidatesSameWidthErrors)
{
    // Both layouts fit in 2 bytes, so an explicit size of 2 cannot choose between them: fatal, pointing at
    // metatile_attribute_fields.
    write_fieldmap_header(R"(
#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF
#define METATILE_ATTR_LAYER_MASK    0xF000
#define METATILE_ATTR_BEHAVIOR_MASK_FRLG 0x01FF
#define METATILE_ATTR_LAYER_MASK_FRLG    0x3000
)");
    write_config("fieldmap:\n  metatile_attribute_size: 2\n");

    const auto result = resolve(test_tileset_name);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("more than one metatile attribute mask layout matching"), std::string::npos) << text;
    EXPECT_NE(text.find("metatile_attribute_fields"), std::string::npos) << text;
}

TEST_F(MetatileAttributeSchemaResolverTest, ExplicitFieldsSkipSelection)
{
    // Explicit fields are the truth: the inferred candidate's masks are ignored in favor of the config, the width
    // derives from the explicit masks, and the disagreement with the source draws the mismatch warning.
    write_fieldmap_header(emerald_defines);
    write_config(R"(
fieldmap:
  metatile_attribute_fields:
    - name: custom
      mask: 0x00F0
)");

    const auto result = resolve(test_tileset_name);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    ASSERT_EQ(result.value().schema.fields().size(), 1U);
    EXPECT_EQ(result.value().schema.fields().front().name(), "custom");
    EXPECT_NE(result.value().fields_origin.find("explicit metatile_attribute_fields"), std::string::npos);
    EXPECT_EQ(result.value().attribute_bytes, 1U); // 0x00F0 fits one byte
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0U);
    EXPECT_TRUE(diag_.warning_tag_counts().contains(metatile_attr_schema_tag));
}

TEST_F(MetatileAttributeSchemaResolverTest, AmbiguousConditionalDefineIsFatalEvenWithExplicitSize)
{
    // An undecidable conditional makes the masks themselves unusable, so an explicit size cannot rescue selection.
    write_fieldmap_header(R"(
#if SOME_UNKNOWN_FLAG
#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF
#else
#define METATILE_ATTR_BEHAVIOR_MASK 0x01FF
#endif
#define METATILE_ATTR_LAYER_MASK 0xF000
)");
    write_config("fieldmap:\n  metatile_attribute_size: 2\n");

    const auto result = resolve(test_tileset_name);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(error_text(result).find("conflicting values"), std::string::npos) << error_text(result);
}

TEST_F(MetatileAttributeSchemaResolverTest, DualLayoutWithExplicitFieldsStillRequiresExplicitSize)
{
    // Explicit fields make mask-set selection advisory, but they do not resolve the width ambiguity: the width is
    // the read stride, and guessing it wrong silently halves a 4-byte project's attributes. Still fatal.
    write_fieldmap_header(dual_defines);
    write_config(explicit_fields_yaml);

    const auto result = resolve(test_tileset_name);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("more than one metatile attribute mask layout"), std::string::npos) << text;
    EXPECT_NE(text.find("fieldmap.metatile_attribute_size"), std::string::npos) << text;
}

TEST_F(MetatileAttributeSchemaResolverTest, InferenceWarningsRouteThroughTheDiagnosticsSink)
{
    // A shift table entry disagreeing with its mask offset produces an inference warning, which the resolver drains
    // into the (filterable) diagnostics sink under the inference tag.
    write_fieldmap_header(R"(
enum
{
    METATILE_ATTRIBUTE_BEHAVIOR,
    METATILE_ATTRIBUTE_LAYER_TYPE,
    METATILE_ATTRIBUTE_COUNT,
};
)");
    write_fieldmap_source(R"(
static const u32 sMetatileAttrMasks[METATILE_ATTRIBUTE_COUNT] = {
    [METATILE_ATTRIBUTE_BEHAVIOR] = 0x00ff,
    [METATILE_ATTRIBUTE_LAYER_TYPE] = 0xf000,
};

static const u8 sMetatileAttrShifts[METATILE_ATTRIBUTE_COUNT] = {
    [METATILE_ATTRIBUTE_BEHAVIOR] = 4,
};
)");

    const auto result = resolve(test_tileset_name);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_TRUE(diag_.warning_tag_counts().contains(metatile_attr_inference_tag));
}

// --- Explicit masks and sizes. ---

TEST_F(MetatileAttributeSchemaResolverTest, ExplicitMaskExceedingExplicitSizeErrors)
{
    // An explicit size is authoritative: a mask that needs a wider word is a fatal misconfiguration.
    write_config(R"(
fieldmap:
  metatile_attribute_size: 2
  metatile_attribute_fields:
    - name: wide
      mask: 0x30000
)");

    const auto result = resolve(test_tileset_name);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("wide"), std::string::npos) << text;
    EXPECT_NE(text.find("metatile attribute size"), std::string::npos) << text;
}

TEST_F(MetatileAttributeSchemaResolverTest, LayerTypeOverrideRelocatesInferredRoleField)
{
    // The layer_type role field participates in the uniform override machinery: overriding its mask relocates the
    // inferred candidate's layer bits (0xF000 becomes 0x0300).
    write_fieldmap_header(emerald_defines);
    write_config(R"(
fieldmap:
  metatile_attribute_field_overrides:
    layer_type:
      mask: 0x0300
)");

    const auto result = resolve(test_tileset_name);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x0300U);
}

TEST_F(MetatileAttributeSchemaResolverTest, ExplicitFieldsWithoutRoleFieldDisableLayerType)
{
    // No disable syntax exists: omitting the role field from an explicit layout is what disables layer types.
    write_config(explicit_fields_yaml);

    const auto result = resolve(test_tileset_name);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0U);
    EXPECT_EQ(result.value().schema.layer_type_field(), nullptr);
}

TEST_F(MetatileAttributeSchemaResolverTest, ExplicitRoleFieldCarriesTheLayerType)
{
    // The end-to-end shape of the new syntax: `role: layer_type` in the YAML field list marks the field that
    // receives layer-type values, at whatever mask the user declares.
    write_config(R"(
fieldmap:
  metatile_attribute_fields:
    - name: behavior
      mask: 0x00FF
    - name: layer_type
      mask: 0x0F00
      role: layer_type
)");

    const auto result = resolve(test_tileset_name);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x0F00U);
    ASSERT_NE(result.value().schema.layer_type_field(), nullptr);
    EXPECT_EQ(result.value().schema.layer_type_field()->name(), "layer_type");
    // The role field is not a value column.
    EXPECT_EQ(result.value().schema.value_fields().size(), 1U);
}

TEST_F(MetatileAttributeSchemaResolverTest, RelocatedLayerMaskComesFromTheSource)
{
    // With the fields inferred, the project's own relocated mask is what lands: there is no vanilla-position
    // default anywhere for it to fall back to.
    write_fieldmap_header(relocated_layer_defines);

    const auto result = resolve(test_tileset_name);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x0F00U);
}

TEST_F(MetatileAttributeSchemaResolverTest, ExplicitFieldsRescueUnusableInferredMasks)
{
    // Both inference failures name metatile_attribute_fields as the remedy, so declaring it has to actually
    // rescue the resolve. The explicit layout is trusted as-is: with no role field declared, layer types are
    // disabled (there is no size-based default anymore).
    write_fieldmap_header(R"(
#if SOME_UNKNOWN_FLAG
#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF
#else
#define METATILE_ATTR_BEHAVIOR_MASK 0x01FF
#endif
#define METATILE_ATTR_LAYER_MASK 0x0F00
)");
    write_config(std::string{explicit_fields_yaml} + "  metatile_attribute_size: 2\n");

    const auto result = resolve(test_tileset_name);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0U);
}

TEST_F(MetatileAttributeSchemaResolverTest, CliSizeOverridesYaml)
{
    // CLI beats the YAML knob. The behavior mask (0x00FF) fits 8 bits, so a 1-byte width is consistent and must
    // resolve without widening.
    write_config(std::string{explicit_fields_yaml} + "  metatile_attribute_size: 4\n");
    CliOptionStorage storage;
    storage.metatile_attribute_size = "1";

    const auto result = resolve_with_cli(test_tileset_name, storage);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 1U);
    EXPECT_EQ(result.value().declaration_bytes, 1U);
}

TEST_F(MetatileAttributeSchemaResolverTest, RoleFieldMaskExceedingKnobIsFatal)
{
    // An explicit size is the read stride: a role-field mask that needs a wider word is a fatal
    // misconfiguration, not a hidden widen.
    write_config(R"(
fieldmap:
  metatile_attribute_size: 2
  metatile_attribute_fields:
    - name: behavior
      mask: 0x00FF
    - name: layer_type
      mask: 0x60000000
      role: layer_type
)");

    const auto result = resolve(test_tileset_name);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("layer_type"), std::string::npos) << text; // names the offending mask
    EXPECT_NE(text.find("metatile attribute size"), std::string::npos) << text;
}

TEST_F(MetatileAttributeSchemaResolverTest, WideRoleFieldMaskDerivesFourByteWidth)
{
    // No header and no knob: the width follows the merged masks, so a 4-byte role mask makes a 4-byte layout
    // with nothing to warn about.
    write_config(R"(
fieldmap:
  metatile_attribute_fields:
    - name: behavior
      mask: 0x00FF
    - name: layer_type
      mask: 0x60000000
      role: layer_type
)");

    const auto result = resolve(test_tileset_name);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 4U);
    EXPECT_FALSE(diag_.warning_tag_counts().contains(metatile_attr_schema_tag));
}

TEST_F(MetatileAttributeSchemaResolverTest, NoHeaderDerivesWidthFromExplicitMasks)
{
    write_config(explicit_fields_yaml);
    // No fieldmap header written: the explicit masks are the only width evidence, and 0x00FF fits one byte.

    const auto result = resolve(test_tileset_name);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 1U);
    EXPECT_EQ(result.value().declaration_bytes, 1U);
    EXPECT_FALSE(diag_.warning_tag_counts().contains(metatile_attr_schema_tag));
}

TEST_F(MetatileAttributeSchemaResolverTest, InvalidKnobValueIsFatal)
{
    // The knob only accepts the widths the engine can read: 1, 2, or 4 bytes.
    write_config(std::string{explicit_fields_yaml} + "  metatile_attribute_size: 3\n");

    const auto result = resolve(test_tileset_name);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("Metatile Attribute Size"), std::string::npos) << text;
    EXPECT_NE(text.find("must be"), std::string::npos) << text;
}

// --- The declaration-size knob. ---

TEST_F(MetatileAttributeSchemaResolverTest, DeclarationSizeInferredFromStructTileset)
{
    // The expansion FRLG shape rebuilt from fixtures: dual defines select 4 bytes, but struct Tileset declares
    // 'const u16 *metatileAttributes', so generated declarations stay 2 bytes wide.
    write_fieldmap_header(std::string{dual_defines} + R"(
struct Tileset
{
    /*0x00*/ bool8 isCompressed;
    /*0x10*/ const u16 *metatileAttributes;
    /*0x14*/ TilesetCB callback;
};
)");
    CliOptionStorage storage;
    storage.metatile_attribute_size = "4";

    const auto result = resolve_with_cli(test_tileset_name, storage);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 4U);
    EXPECT_EQ(result.value().declaration_bytes, 2U);
}

TEST_F(MetatileAttributeSchemaResolverTest, ExplicitDeclarationSizeOverridesStructInference)
{
    write_fieldmap_header(std::string{emerald_defines} + R"(
struct Tileset
{
    /*0x10*/ const u16 *metatileAttributes;
};
)");
    write_config("fieldmap:\n  metatile_attribute_declaration_size: 4\n");

    const auto result = resolve(test_tileset_name);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 2U);
    EXPECT_EQ(result.value().declaration_bytes, 4U);
}

TEST_F(MetatileAttributeSchemaResolverTest, MaskWidthBelowScannedDeclarationIsFatal)
{
    // Every mask sits in the low byte, but struct Tileset stores attributes in a u16 array. Masks prove a minimum
    // width, never the width itself, so the two facts conflict and only the explicit knob can resolve which width
    // the project actually reads.
    write_fieldmap_header(R"(
#define METATILE_ATTR_BEHAVIOR_MASK 0x003F
#define METATILE_ATTR_LAYER_MASK    0x00C0

struct Tileset
{
    /*0x10*/ const u16 *metatileAttributes;
};
)");

    const auto result = resolve(test_tileset_name);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("struct Tileset"), std::string::npos) << text;
    EXPECT_NE(text.find("fieldmap.metatile_attribute_size"), std::string::npos) << text;
}

TEST_F(MetatileAttributeSchemaResolverTest, KnobResolvesMaskVersusDeclarationConflict)
{
    // The same shape as MaskWidthBelowScannedDeclarationIsFatal, with the knob supplying the answer. The scanned
    // declaration corroborates the knob, so no wider-knob warning fires.
    write_fieldmap_header(R"(
#define METATILE_ATTR_BEHAVIOR_MASK 0x003F
#define METATILE_ATTR_LAYER_MASK    0x00C0

struct Tileset
{
    /*0x10*/ const u16 *metatileAttributes;
};
)");
    write_config("fieldmap:\n  metatile_attribute_size: 2\n");

    const auto result = resolve(test_tileset_name);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 2U);
    EXPECT_EQ(result.value().declaration_bytes, 2U);
    ASSERT_NE(schema_field(result.value().schema, "behavior"), nullptr);
    EXPECT_EQ(schema_field(result.value().schema, "behavior")->mask(), 0x003FU);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x00C0U);
    EXPECT_FALSE(diag_.warning_tag_counts().contains(metatile_attr_schema_tag));
}

TEST_F(MetatileAttributeSchemaResolverTest, UnsetDeclarationSizeMatchesAttributeBytes)
{
    // No struct Tileset and no knob: the declaration width follows the resolved attribute width.
    write_fieldmap_header(emerald_defines);

    const auto result = resolve(test_tileset_name);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 2U);
    EXPECT_EQ(result.value().declaration_bytes, 2U);
}

TEST_F(MetatileAttributeSchemaResolverTest, InvalidDeclarationSizeIsFatal)
{
    write_config(std::string{explicit_fields_yaml} + "  metatile_attribute_declaration_size: 3\n");

    const auto result = resolve(test_tileset_name);
    ASSERT_FALSE(result.has_value());
    const auto text = error_text(result);
    EXPECT_NE(text.find("Metatile Attribute Declaration Size"), std::string::npos) << text;
    EXPECT_NE(text.find("must be"), std::string::npos) << text;
}

// --- Acceptance cases A-D against the local decomp checkouts, when present. ---

TEST_F(MetatileAttributeSchemaResolverTest, AcceptanceCaseAPokeemeraldZeroConfig)
{
    const std::filesystem::path root = "./pokeemerald";
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "pokeemerald testbed not present";
    }

    const auto result = resolve_impl(root, "gTileset_General", nullptr);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 2U);
    EXPECT_EQ(result.value().declaration_bytes, 2U);
    ASSERT_EQ(result.value().schema.fields().size(), 2U);
    EXPECT_EQ(result.value().schema.fields().front().name(), "behavior");
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0xF000U);
}

TEST_F(MetatileAttributeSchemaResolverTest, AcceptanceCaseBPokefireredZeroConfig)
{
    const std::filesystem::path root = "./pokefirered";
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "pokefirered testbed not present";
    }

    const auto result = resolve_impl(root, "gTileset_General", nullptr);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 4U);
    EXPECT_EQ(result.value().declaration_bytes, 4U);
    EXPECT_EQ(result.value().schema.fields().size(), 8U);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x60000000U);
}

TEST_F(MetatileAttributeSchemaResolverTest, AcceptanceCaseCExpansionEmeraldFlavor)
{
    const std::filesystem::path root = "./pokeemerald-expansion";
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "pokeemerald-expansion testbed not present";
    }

    CliOptionStorage storage;
    storage.metatile_attribute_size = "2";
    const auto result = resolve_impl(root, "gTileset_General", &storage);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 2U);
    EXPECT_EQ(result.value().declaration_bytes, 2U);
    ASSERT_EQ(result.value().schema.fields().size(), 2U);
    EXPECT_EQ(result.value().schema.fields().front().name(), "behavior");
    EXPECT_EQ(result.value().schema.fields().front().mask(), 0x00FFU);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0xF000U);
}

TEST_F(MetatileAttributeSchemaResolverTest, AcceptanceCaseDExpansionFrlgFlavor)
{
    const std::filesystem::path root = "./pokeemerald-expansion";
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "pokeemerald-expansion testbed not present";
    }

    CliOptionStorage storage;
    storage.metatile_attribute_size = "4";
    const auto result = resolve_impl(root, "gTileset_General", &storage);
    ASSERT_TRUE(result.has_value()) << error_text(result);
    EXPECT_EQ(result.value().attribute_bytes, 4U);
    // struct Tileset declares 'const u16 *metatileAttributes' even for the FRLG flavor.
    EXPECT_EQ(result.value().declaration_bytes, 2U);
    EXPECT_EQ(result.value().schema.fields().size(), 8U);
    ASSERT_NE(schema_field(result.value().schema, "behavior"), nullptr);
    EXPECT_EQ(schema_field(result.value().schema, "behavior")->mask(), 0x1FFU);
    EXPECT_EQ(result.value().schema.layer_type_mask(), 0x60000000U);
}

TEST_F(MetatileAttributeSchemaResolverTest, ExpansionZeroConfigIsFatalNamingTheSizeKnob)
{
    const std::filesystem::path root = "./pokeemerald-expansion";
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "pokeemerald-expansion testbed not present";
    }

    const auto result = resolve_impl(root, "gTileset_General", nullptr);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(error_text(result).find("fieldmap.metatile_attribute_size"), std::string::npos) << error_text(result);
}

} // namespace
} // namespace porytiles
