#include "porytiles/infra/services/tileset_attribute_schema_cache.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "porytiles/infra/config/default_provider.hpp"
#include "porytiles/infra/config/lazy_layered_config.hpp"
#include "porytiles/infra/config/metatiles_header_provider.hpp"
#include "porytiles/infra/config/yaml_file_provider.hpp"
#include "porytiles/infra/services/project_layout_metadata_provider.hpp"
#include "porytiles/infra/services/tileset_attribute_schema_resolver.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

namespace porytiles {
namespace {

// A field set whose two layouts differ observably: the primary layout carries only behavior, while the FRLG layout
// adds terrain.
constexpr auto kDivergentFieldsYaml = R"(
fieldmap:
  metatile_attribute_fields:
    - name: behavior
      mask: 0x00FF
      frlg_mask: 0x01FF
    - name: terrain
      frlg_mask: 0x0E00
)";

class TilesetAttributeSchemaCacheTest : public ::testing::Test {
  protected:
    std::filesystem::path project_root_;
    PlainTextFormatter formatter_;
    BufferedUserDiagnostics diag_;

    // Owns the full provider/config/resolver stack the cache points into, so a test can build one on the stack and
    // keep every dependency alive while it holds entry pointers.
    struct Harness {
        Harness(const std::filesystem::path &root, TextFormatter *format, const UserDiagnostics *diag)
            : config{format, make_config_providers(root, format, diag)}, layout_metadata{root, format, diag},
              metatiles_header{root, format}, resolver{&config, &layout_metadata, &metatiles_header, format, diag},
              cache{root, &resolver, format, diag}
        {
        }

        [[nodiscard]] static std::vector<std::unique_ptr<ConfigProvider>>
        make_config_providers(const std::filesystem::path &root, TextFormatter *format, const UserDiagnostics *diag)
        {
            std::vector<std::unique_ptr<ConfigProvider>> providers{};
            providers.push_back(std::make_unique<YamlFileProvider>(format, diag, root));
            providers.push_back(std::make_unique<DefaultProvider>());
            return providers;
        }

        LazyLayeredConfig config;
        ProjectLayoutMetadataProvider layout_metadata;
        MetatilesHeaderProvider metatiles_header;
        TilesetAttributeSchemaResolver resolver;
        TilesetAttributeSchemaCache cache;
    };

    void SetUp() override
    {
        // YamlFileProvider caches parsed files by path process-wide, so each test needs a distinct root.
        const auto *info = ::testing::UnitTest::GetInstance()->current_test_info();
        project_root_ = std::filesystem::temp_directory_path() /
                        (std::string{"porytiles_schema_cache_test_"} + info->test_suite_name() + "_" + info->name());
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

    void write_tileset_config(const std::string &tileset_name, const std::string &yaml)
    {
        const auto dir = project_root_ / "porytiles" / "tilesets" / tileset_name;
        std::filesystem::create_directories(dir);
        std::ofstream out{dir / "config.yaml"};
        out << yaml;
    }

    [[nodiscard]] std::string error_text(const ChainableResult<const TilesetAttributeSchemaCache::Entry *> &result)
    {
        std::string text;
        for (const auto &err : result.chain()) {
            text += err->join(formatter_);
            text += "\n";
        }
        return text;
    }
};

TEST_F(TilesetAttributeSchemaCacheTest, EntryResolvesOnceAndReturnsStablePointer)
{
    write_config(kDivergentFieldsYaml);
    Harness harness{project_root_, &formatter_, &diag_};

    const auto first = harness.cache.entry("gTileset_Test");
    ASSERT_TRUE(first.has_value()) << error_text(first);
    const auto second = harness.cache.entry("gTileset_Test");
    ASSERT_TRUE(second.has_value()) << error_text(second);

    EXPECT_EQ(first.value(), second.value());

    // The resolver's summary remark fires once per tileset, proving the second call hit the cache.
    ASSERT_TRUE(diag_.remark_tag_counts().contains("metatile-attr-schema"));
    EXPECT_EQ(diag_.remark_tag_counts().at("metatile-attr-schema"), 1u);
}

// The reason the cache exists: two tilesets in the same project can resolve different schemas, and each entry must
// reflect its own tileset's resolution rather than whichever tileset was resolved first.
TEST_F(TilesetAttributeSchemaCacheTest, TilesetsResolveTheirOwnSchemas)
{
    write_config(kDivergentFieldsYaml);
    write_tileset_config("gTileset_Secondary", "fieldmap:\n  use_frlg_alternate_masks: always\n");
    Harness harness{project_root_, &formatter_, &diag_};

    const auto primary = harness.cache.entry("gTileset_Primary");
    ASSERT_TRUE(primary.has_value()) << error_text(primary);
    const auto secondary = harness.cache.entry("gTileset_Secondary");
    ASSERT_TRUE(secondary.has_value()) << error_text(secondary);

    EXPECT_EQ(primary.value()->resolved.layout, AttributeSchemaLayout::primary);
    ASSERT_EQ(primary.value()->resolved.schema.fields().size(), 1U);
    EXPECT_EQ(primary.value()->resolved.schema.fields()[0].name(), "behavior");

    EXPECT_EQ(secondary.value()->resolved.layout, AttributeSchemaLayout::frlg);
    ASSERT_EQ(secondary.value()->resolved.schema.fields().size(), 2U);
    EXPECT_EQ(secondary.value()->resolved.schema.fields()[1].name(), "terrain");
}

TEST_F(TilesetAttributeSchemaCacheTest, EntryProvidersUpholdTheMembershipContract)
{
    write_config(R"(
fieldmap:
  metatile_attribute_fields:
    - name: behavior
      mask: 0x00FF
      provider:
        header: include/constants/metatile_behaviors.h
        prefix: MB_
    - name: terrain
      mask: 0x0E00
)");
    Harness harness{project_root_, &formatter_, &diag_};

    const auto entry = harness.cache.entry("gTileset_Test");
    ASSERT_TRUE(entry.has_value()) << error_text(entry);

    // Exactly the provider-backed fields get providers: behavior does, the raw terrain field does not.
    EXPECT_EQ(entry.value()->providers.size(), 1U);
    EXPECT_TRUE(entry.value()->providers.contains("behavior"));
    EXPECT_FALSE(entry.value()->providers.contains("terrain"));
}

TEST_F(TilesetAttributeSchemaCacheTest, ResolutionFailurePropagates)
{
    // A field with neither mask is a hard resolution error.
    write_config(R"(
fieldmap:
  metatile_attribute_fields:
    - name: broken
)");
    Harness harness{project_root_, &formatter_, &diag_};

    const auto entry = harness.cache.entry("gTileset_Test");
    EXPECT_FALSE(entry.has_value());
}

} // namespace
} // namespace porytiles
