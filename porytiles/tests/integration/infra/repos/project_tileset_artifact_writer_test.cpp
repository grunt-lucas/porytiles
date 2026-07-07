#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "porytiles/domain/models/image.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/models/metatile_attribute_schema.hpp"
#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/porymap_tileset_component.hpp"
#include "porytiles/domain/models/porytiles_tileset_component.hpp"
#include "porytiles/domain/models/rgba32.hpp"
#include "porytiles/domain/models/tilemap_entry.hpp"
#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/domain/services/enum_map_provider.hpp"
#include "porytiles/infra/repos/project_tileset_artifact_writer.hpp"
#include "porytiles/infra/services/anim_code_generator.hpp"
#include "porytiles/infra/services/anim_json_parser.hpp"
#include "porytiles/infra/services/file_pal_saver.hpp"
#include "porytiles/infra/services/png_indexed_image_saver.hpp"
#include "porytiles/infra/services/png_rgba_image_saver.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

#include "support/mock_domain_config.hpp"
#include "support/mock_infra_config.hpp"

using namespace porytiles;

namespace {

class MockPngRgbaImageSaver : public PngRgbaImageSaver {
  public:
    [[nodiscard]] ChainableResult<void>
    save_to_file(const Image<Rgba32> &image, const std::filesystem::path &path) const override
    {
        std::ofstream out{path};
        out << "mock_rgba_image";
        return {};
    }
};

class MockPngIndexedImageSaver : public PngIndexedImageSaver {
  public:
    [[nodiscard]] ChainableResult<void>
    save_to_file(const Image<IndexPixel> &image, const std::filesystem::path &path, TilesPalMode mode) const override
    {
        std::ofstream out{path};
        out << "mock_indexed_image";
        return {};
    }
};

class MockFilePalSaver : public FilePalSaver {
  public:
    [[nodiscard]] ChainableResult<void>
    save(const Palette<Rgba32, pal::max_size> &pal, const std::filesystem::path &path) const override
    {
        std::ofstream out{path};
        out << "mock_palette";
        return {};
    }
};

class MockBehaviorMapProvider : public EnumMapProvider {
  public:
    [[nodiscard]] ChainableResult<std::uint32_t> lookup(const std::string &name) const override
    {
        // Simple mock: return 0 for any behavior name
        return std::uint32_t{0};
    }

    [[nodiscard]] ChainableResult<std::string> lookup(std::uint32_t value) const override
    {
        // Simple mock: return a behavior name based on value
        return std::string{"MB_NORMAL"};
    }
};

// A stub with a real value<->name mapping, for the multi-field tests that must render distinct provider names.
class StubEnumMapProvider final : public EnumMapProvider {
  public:
    explicit StubEnumMapProvider(std::unordered_map<std::string, std::uint32_t> name_to_value)
        : name_to_value_{std::move(name_to_value)}
    {
        for (const auto &[name, value] : name_to_value_) {
            value_to_name_[value] = name;
        }
    }

    [[nodiscard]] ChainableResult<std::uint32_t> lookup(const std::string &name) const override
    {
        auto it = name_to_value_.find(name);
        if (it == name_to_value_.end()) {
            return FormattableError{"unknown name: {}", FormatParam{name}};
        }
        return it->second;
    }

    [[nodiscard]] ChainableResult<std::string> lookup(std::uint32_t value) const override
    {
        auto it = value_to_name_.find(value);
        if (it == value_to_name_.end()) {
            return FormattableError{"unknown value: {}", FormatParam{value}};
        }
        return it->second;
    }

  private:
    std::unordered_map<std::string, std::uint32_t> name_to_value_{};
    std::unordered_map<std::uint32_t, std::string> value_to_name_{};
};

// The ProviderSpec contents are irrelevant here (the tests stub the ProviderMap directly); the spec's presence is what
// marks a field provider-backed.
ProviderSpec dummy_provider_spec()
{
    return ProviderSpec{.header = "include/dummy.h", .prefix = "DUMMY_"};
}

// The stock emerald shape: a single provider-backed behavior field in a 2-byte attribute.
Schema make_emerald_schema()
{
    auto result = Schema::create({Field{"behavior", 0x00FF, 0, dummy_provider_spec()}}, 2);
    return std::move(result).value();
}

ProviderMap make_emerald_provider_map()
{
    ProviderMap providers{};
    providers.emplace("behavior", std::make_unique<MockBehaviorMapProvider>());
    return providers;
}

// The stock firered shape: seven fields in a 4-byte attribute, three provider-backed and four raw. Masks match the
// FRLG attribute bit layout from fieldmap.c (layer_type is structural and never a schema field).
Schema make_firered_schema()
{
    auto result = Schema::create(
        {
            Field{"behavior", 0x000001FF, 0, dummy_provider_spec()},
            Field{"terrain", 0x00003E00, 0, dummy_provider_spec()},
            Field{"attribute_2", 0x0003C000},
            Field{"attribute_3", 0x00FC0000},
            Field{"encounter_type", 0x07000000, 0, dummy_provider_spec()},
            Field{"attribute_5", 0x18000000},
            Field{"attribute_7", 0x80000000},
        },
        4);
    return std::move(result).value();
}

ProviderMap make_firered_provider_map()
{
    ProviderMap providers{};
    providers.emplace(
        "behavior",
        std::make_unique<StubEnumMapProvider>(
            std::unordered_map<std::string, std::uint32_t>{{"MB_NORMAL", 0}, {"MB_TALL_GRASS", 2}}));
    providers.emplace(
        "terrain",
        std::make_unique<StubEnumMapProvider>(
            std::unordered_map<std::string, std::uint32_t>{{"TILE_TERRAIN_NORMAL", 0}, {"TILE_TERRAIN_GRASS", 1}}));
    providers.emplace(
        "encounter_type",
        std::make_unique<StubEnumMapProvider>(
            std::unordered_map<std::string, std::uint32_t>{{"TILE_ENCOUNTER_NONE", 0}, {"TILE_ENCOUNTER_LAND", 1}}));
    return providers;
}

Image<Rgba32> create_test_rgba_image()
{
    return Image<Rgba32>{8, 8};
}

Image<IndexPixel> create_test_indexed_image()
{
    return Image<IndexPixel>{8, 8};
}

Tileset create_test_tileset(const std::string &name)
{
    auto porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    porytiles_component->bottom(create_test_rgba_image());
    porytiles_component->middle(create_test_rgba_image());
    porytiles_component->top(create_test_rgba_image());

    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    porymap_component->tiles_png(create_test_indexed_image());

    for (std::uint32_t i = 0; i < 64; i++) {
        porymap_component->push_back_tilemap_entry(TilemapEntry{i, false, false, false});
    }

    for (int i = 0; i < 16; i++) {
        porymap_component->set_pal(i, Palette<Rgba32, pal::max_size>{Rgba32{0, 0, 0, Rgba32::alpha_opaque}});
    }

    for (int i = 0; i < 8; i++) {
        porymap_component->push_back_attribute(MetatileAttribute{});
    }

    return Tileset{name, std::move(porytiles_component), std::move(porymap_component)};
}

// A tileset whose bottom layer holds two metatiles (32x16 px) but whose Porytiles attribute map stores only metatile 0.
// This is the sparse case the layerType writer must materialize a full row set for.
Tileset create_sparse_two_metatile_tileset(const std::string &name)
{
    auto porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    porytiles_component->bottom(Image<Rgba32>{32, 16}); // (32/16) * (16/16) = 2 metatiles
    porytiles_component->middle(Image<Rgba32>{32, 16});
    porytiles_component->top(Image<Rgba32>{32, 16});

    // Only metatile 0 has a stored attribute: a non-default behavior and an explicit "covered" layer type.
    MetatileAttribute attr_0{};
    attr_0.field(attr::field_behavior, 5);
    attr_0.explicit_layer_type(LayerType::covered);
    porytiles_component->insert_attribute(0, attr_0);

    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    porymap_component->tiles_png(create_test_indexed_image());

    return Tileset{name, std::move(porytiles_component), std::move(porymap_component)};
}

// Like create_sparse_two_metatile_tileset, but metatile 0's layer type is set through the plain layer_type() setter
// (as a bin parser or the CSV loader does for an inferred/auto row) rather than explicit_layer_type(). This is the
// round-trip case: an auto row must never be written back as a pinned token, no matter what layer_type() happens to be.
Tileset create_two_metatile_tileset_with_inferred_layer_type(const std::string &name)
{
    auto porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    porytiles_component->bottom(Image<Rgba32>{32, 16}); // 2 metatiles
    porytiles_component->middle(Image<Rgba32>{32, 16});
    porytiles_component->top(Image<Rgba32>{32, 16});

    // Non-default behavior so the row is meaningful, and a non-'normal' layer type set the inferred way (no explicit).
    MetatileAttribute attr_0{};
    attr_0.field(attr::field_behavior, 5);
    attr_0.layer_type(LayerType::covered);
    porytiles_component->insert_attribute(0, attr_0);

    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    porymap_component->tiles_png(create_test_indexed_image());

    return Tileset{name, std::move(porytiles_component), std::move(porymap_component)};
}

[[nodiscard]] std::string read_whole_file(const std::filesystem::path &path)
{
    std::ifstream in{path};
    return std::string{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
}

} // namespace

class ProjectTilesetArtifactWriterTests : public ::testing::Test {
  protected:
    void SetUp() override
    {
        formatter_ = std::make_unique<PlainTextFormatter>();
        diag_ = std::make_unique<BufferedUserDiagnostics>();
        domain_config_ = std::make_unique<MockDomainConfig>();
        infra_config_ = std::make_unique<MockInfraConfig>();
        png_rgba_saver_ = std::make_unique<MockPngRgbaImageSaver>();
        png_indexed_saver_ = std::make_unique<MockPngIndexedImageSaver>();
        pal_saver_ = std::make_unique<MockFilePalSaver>();
        anim_json_parser_ = std::make_unique<AnimJsonParser>(formatter_.get());
        anim_code_generator_ = std::make_unique<AnimCodeGenerator>();

        test_root_ = std::filesystem::temp_directory_path() / "porytiles_artifact_writer_tests";
        std::filesystem::create_directories(test_root_);

        writer_ = std::make_unique<ProjectTilesetArtifactWriter>(
            domain_config_.get(),
            infra_config_.get(),
            test_root_,
            &schema_,
            &providers_,
            formatter_.get(),
            diag_.get(),
            png_rgba_saver_.get(),
            png_indexed_saver_.get(),
            pal_saver_.get(),
            anim_json_parser_.get(),
            anim_code_generator_.get());
    }

    void TearDown() override
    {
        if (std::filesystem::exists(test_root_)) {
            std::filesystem::remove_all(test_root_);
        }
    }

    [[nodiscard]] bool file_contains(const std::filesystem::path &path, const std::string &content) const
    {
        if (!std::filesystem::exists(path)) {
            return false;
        }
        std::ifstream in{path};
        std::string file_content;
        std::getline(in, file_content);
        return file_content == content;
    }

    [[nodiscard]] bool file_exists_and_not_empty(const std::filesystem::path &path) const
    {
        return std::filesystem::exists(path) && std::filesystem::file_size(path) > 0;
    }

    [[nodiscard]] std::size_t count_temp_dirs() const
    {
        std::size_t count = 0;
        for (const auto &entry : std::filesystem::directory_iterator(test_root_)) {
            if (entry.is_directory() && entry.path().filename().string().starts_with(".porytiles_tmp_")) {
                count++;
            }
        }
        return count;
    }

    std::filesystem::path test_root_;
    std::unique_ptr<TextFormatter> formatter_;
    std::unique_ptr<UserDiagnostics> diag_;
    std::unique_ptr<MockDomainConfig> domain_config_;
    std::unique_ptr<MockInfraConfig> infra_config_;
    std::unique_ptr<MockPngRgbaImageSaver> png_rgba_saver_;
    std::unique_ptr<MockPngIndexedImageSaver> png_indexed_saver_;
    std::unique_ptr<MockFilePalSaver> pal_saver_;
    std::unique_ptr<AnimJsonParser> anim_json_parser_;
    std::unique_ptr<AnimCodeGenerator> anim_code_generator_;
    Schema schema_ = make_emerald_schema();
    ProviderMap providers_ = make_emerald_provider_map();
    std::unique_ptr<ProjectTilesetArtifactWriter> writer_;
};

TEST_F(ProjectTilesetArtifactWriterTests, BasicTransactionLifecycle)
{
    auto tileset = create_test_tileset("test_tileset");

    auto begin_result = writer_->begin_transaction();
    ASSERT_TRUE(begin_result.has_value());

    auto expected_file = test_root_ / "tileset_bottom.png";
    ArtifactKey key{"tileset_bottom.png"};
    auto write_result = writer_->write_bottom_png(key, tileset);
    if (!write_result.has_value()) {
        FAIL() << "Write error: " << write_result.error().join(PlainTextFormatter{});
    }

    auto commit_result = writer_->commit();
    ASSERT_TRUE(commit_result.has_value());
    ASSERT_TRUE(std::filesystem::exists(expected_file));
    ASSERT_TRUE(file_contains(expected_file, "mock_rgba_image"));
}

TEST_F(ProjectTilesetArtifactWriterTests, RollbackTransaction)
{
    auto tileset = create_test_tileset("test_tileset");
    auto initial_temp_count = count_temp_dirs();

    auto begin_result = writer_->begin_transaction();
    ASSERT_TRUE(begin_result.has_value());

    auto expected_file = test_root_ / "rollback_test.png";
    ArtifactKey key{"rollback_test.png"};
    auto write_result = writer_->write_bottom_png(key, tileset);
    ASSERT_TRUE(write_result.has_value());

    auto rollback_result = writer_->rollback();
    ASSERT_FALSE(std::filesystem::exists(expected_file));

    ASSERT_EQ(count_temp_dirs(), initial_temp_count);
}

TEST_F(ProjectTilesetArtifactWriterTests, MultipleWritesInTransaction)
{
    auto tileset = create_test_tileset("test_tileset");

    auto begin_result = writer_->begin_transaction();
    ASSERT_TRUE(begin_result.has_value());

    ArtifactKey key1{"assets/bottom.png"};
    auto write_result1 = writer_->write_bottom_png(key1, tileset);
    ASSERT_TRUE(write_result1.has_value());

    ArtifactKey key2{"assets/middle.png"};
    auto write_result2 = writer_->write_middle_png(key2, tileset);
    ASSERT_TRUE(write_result2.has_value());

    ArtifactKey key3{"assets/tiles.png"};
    auto write_result3 = writer_->write_tiles_png(key3, tileset);
    ASSERT_TRUE(write_result3.has_value());

    auto commit_result = writer_->commit();
    ASSERT_TRUE(commit_result.has_value());

    ASSERT_TRUE(std::filesystem::exists(test_root_ / "assets/bottom.png"));
    ASSERT_TRUE(std::filesystem::exists(test_root_ / "assets/middle.png"));
    ASSERT_TRUE(std::filesystem::exists(test_root_ / "assets/tiles.png"));
    ASSERT_TRUE(file_contains(test_root_ / "assets/bottom.png", "mock_rgba_image"));
    ASSERT_TRUE(file_contains(test_root_ / "assets/middle.png", "mock_rgba_image"));
    ASSERT_TRUE(file_contains(test_root_ / "assets/tiles.png", "mock_indexed_image"));
}

TEST_F(ProjectTilesetArtifactWriterTests, OverwriteExistingFiles)
{
    auto tileset = create_test_tileset("test_tileset");

    auto existing_file = test_root_ / "existing.png";
    std::ofstream out{existing_file};
    out << "original_content";
    out.close();

    ASSERT_TRUE(file_contains(existing_file, "original_content"));

    auto begin_result = writer_->begin_transaction();
    ASSERT_TRUE(begin_result.has_value());

    ArtifactKey key{"existing.png"};
    auto write_result = writer_->write_bottom_png(key, tileset);
    ASSERT_TRUE(write_result.has_value());

    auto commit_result = writer_->commit();
    ASSERT_TRUE(commit_result.has_value());

    ASSERT_TRUE(std::filesystem::exists(existing_file));
    ASSERT_TRUE(file_contains(existing_file, "mock_rgba_image"));
}

TEST_F(ProjectTilesetArtifactWriterTests, NestedDirectoryStructure)
{
    auto tileset = create_test_tileset("test_tileset");

    auto begin_result = writer_->begin_transaction();
    ASSERT_TRUE(begin_result.has_value());

    auto expected_file = test_root_ / "deeply/nested/directory/structure/file.png";
    ArtifactKey key{"deeply/nested/directory/structure/file.png"};
    auto write_result = writer_->write_top_png(key, tileset);
    ASSERT_TRUE(write_result.has_value());

    auto commit_result = writer_->commit();
    ASSERT_TRUE(commit_result.has_value());
    ASSERT_TRUE(std::filesystem::exists(expected_file));
    ASSERT_TRUE(file_contains(expected_file, "mock_rgba_image"));
}

TEST_F(ProjectTilesetArtifactWriterTests, NoTransactionInProgress)
{
    auto tileset = create_test_tileset("test_tileset");

    auto commit_result = writer_->commit();
    ASSERT_FALSE(commit_result.has_value());
    auto commit_error_lines = commit_result.error().details(PlainTextFormatter{});
    ASSERT_EQ(commit_error_lines.size(), 1);
    EXPECT_EQ(commit_error_lines[0], "No transaction in progress.");

    auto rollback_result = writer_->rollback();
    ASSERT_FALSE(rollback_result.has_value());
    EXPECT_EQ(rollback_result.error().details(PlainTextFormatter{}).at(0), "No transaction in progress.");

    ArtifactKey key{"no_transaction.png"};
    auto write_result = writer_->write_bottom_png(key, tileset);
    ASSERT_FALSE(write_result.has_value());
    auto error_lines = write_result.error().details(PlainTextFormatter{});
    // The new specific write method chains the error through compute_transaction_dest_path
    ASSERT_GE(error_lines.size(), 1);
    EXPECT_EQ(error_lines[0], "Failed to compute transaction dest path.");
}

TEST_F(ProjectTilesetArtifactWriterTests, DoubleBeginTransaction)
{
    auto begin_result1 = writer_->begin_transaction();
    ASSERT_TRUE(begin_result1.has_value());

    auto begin_result2 = writer_->begin_transaction();
    ASSERT_FALSE(begin_result2.has_value());
    EXPECT_EQ(begin_result2.error().details(PlainTextFormatter{}).at(0), "Transaction already in progress.");

    auto rollback_result = writer_->rollback();
    ASSERT_TRUE(rollback_result.has_value());
}

TEST_F(ProjectTilesetArtifactWriterTests, CommitCleansUpTempDirectory)
{
    auto tileset = create_test_tileset("test_tileset");
    auto initial_temp_count = count_temp_dirs();

    auto begin_result = writer_->begin_transaction();
    ASSERT_TRUE(begin_result.has_value());

    ASSERT_GT(count_temp_dirs(), initial_temp_count);

    ArtifactKey key{"temp_cleanup.png"};
    auto write_result = writer_->write_bottom_png(key, tileset);
    ASSERT_TRUE(write_result.has_value());

    auto commit_result = writer_->commit();
    ASSERT_TRUE(commit_result.has_value());

    ASSERT_EQ(count_temp_dirs(), initial_temp_count);
}

TEST_F(ProjectTilesetArtifactWriterTests, RollbackCleansUpTempDirectory)
{
    auto tileset = create_test_tileset("test_tileset");
    auto initial_temp_count = count_temp_dirs();

    auto begin_result = writer_->begin_transaction();
    ASSERT_TRUE(begin_result.has_value());

    ASSERT_GT(count_temp_dirs(), initial_temp_count);

    auto rollback_result = writer_->rollback();
    ASSERT_TRUE(rollback_result.has_value());

    ASSERT_EQ(count_temp_dirs(), initial_temp_count);
}

TEST_F(ProjectTilesetArtifactWriterTests, WriteMetatilesBin)
{
    auto tileset = create_test_tileset("test_tileset");

    auto begin_result = writer_->begin_transaction();
    ASSERT_TRUE(begin_result.has_value());

    auto expected_file = test_root_ / "metatiles.bin";
    ArtifactKey key{"metatiles.bin"};
    auto write_result = writer_->write_metatiles_bin(key, tileset);
    ASSERT_TRUE(write_result.has_value());

    auto commit_result = writer_->commit();
    ASSERT_TRUE(commit_result.has_value());
    ASSERT_TRUE(std::filesystem::exists(expected_file));
    ASSERT_GT(std::filesystem::file_size(expected_file), 0);
}

TEST_F(ProjectTilesetArtifactWriterTests, WriteMetatileAttributesBin)
{
    auto tileset = create_test_tileset("test_tileset");

    auto begin_result = writer_->begin_transaction();
    ASSERT_TRUE(begin_result.has_value());

    auto expected_file = test_root_ / "metatile_attributes.bin";
    ArtifactKey key{"metatile_attributes.bin"};
    auto write_result = writer_->write_metatile_attributes_bin(key, tileset);
    ASSERT_TRUE(write_result.has_value());

    auto commit_result = writer_->commit();
    ASSERT_TRUE(commit_result.has_value());
    ASSERT_TRUE(std::filesystem::exists(expected_file));
    ASSERT_EQ(std::filesystem::file_size(expected_file), 16);
}

// A metatile_attr_field_overrides mask change must reach metatile_attributes.bin, not just the CSV: the same
// attribute written under two schemas differing only in the behavior mask produces different bytes, each matching
// the schema's layout exactly.
TEST_F(ProjectTilesetArtifactWriterTests, WriteMetatileAttributesBinFollowsSchemaMasks)
{
    auto make_writer = [&](const Schema &schema, const ProviderMap &providers) {
        return ProjectTilesetArtifactWriter{
            domain_config_.get(),
            infra_config_.get(),
            test_root_,
            &schema,
            &providers,
            formatter_.get(),
            diag_.get(),
            png_rgba_saver_.get(),
            png_indexed_saver_.get(),
            pal_saver_.get(),
            anim_json_parser_.get(),
            anim_code_generator_.get()};
    };

    auto make_tileset = [] {
        auto porytiles_component = std::make_unique<PorytilesTilesetComponent>();
        auto porymap_component = std::make_unique<PorymapTilesetComponent>();
        MetatileAttribute attribute{};
        attribute.field(attr::field_behavior, 5);
        porymap_component->push_back_attribute(attribute);
        return Tileset{"test_tileset", std::move(porytiles_component), std::move(porymap_component)};
    };

    auto read_file_bytes = [](const std::filesystem::path &path) {
        std::ifstream in{path, std::ios::binary};
        return std::vector<char>{std::istreambuf_iterator<char>{in}, std::istreambuf_iterator<char>{}};
    };

    ProviderMap providers{};

    // Stock mask: behavior at bits 0-7, so value 5 lands in the low byte.
    Schema stock_schema = std::move(Schema::create({Field{"behavior", 0x00FF}}, 2)).value();
    {
        auto writer = make_writer(stock_schema, providers);
        auto tileset = make_tileset();
        ASSERT_TRUE(writer.begin_transaction().has_value());
        ASSERT_TRUE(writer.write_metatile_attributes_bin(ArtifactKey{"stock.bin"}, tileset).has_value());
        ASSERT_TRUE(writer.commit().has_value());
    }
    EXPECT_EQ(read_file_bytes(test_root_ / "stock.bin"), (std::vector<char>{0x05, 0x00}));

    // Overridden mask: behavior shifted to bits 4-11, so the same value 5 lands at offset 4 (0x50).
    Schema shifted_schema = std::move(Schema::create({Field{"behavior", 0x0FF0}}, 2)).value();
    {
        auto writer = make_writer(shifted_schema, providers);
        auto tileset = make_tileset();
        ASSERT_TRUE(writer.begin_transaction().has_value());
        ASSERT_TRUE(writer.write_metatile_attributes_bin(ArtifactKey{"shifted.bin"}, tileset).has_value());
        ASSERT_TRUE(writer.commit().has_value());
    }
    EXPECT_EQ(read_file_bytes(test_root_ / "shifted.bin"), (std::vector<char>{0x50, 0x00}));
}

TEST_F(ProjectTilesetArtifactWriterTests, WritePalette)
{
    auto tileset = create_test_tileset("test_tileset");

    auto begin_result = writer_->begin_transaction();
    ASSERT_TRUE(begin_result.has_value());

    auto expected_file = test_root_ / "palette_0.pal";
    ArtifactKey key{"palette_0.pal"};
    auto write_result = writer_->write_porymap_pal_n(key, tileset, 0);
    ASSERT_TRUE(write_result.has_value());

    auto commit_result = writer_->commit();
    ASSERT_TRUE(commit_result.has_value());
    ASSERT_TRUE(std::filesystem::exists(expected_file));
    ASSERT_TRUE(file_contains(expected_file, "mock_palette"));
}

TEST_F(ProjectTilesetArtifactWriterTests, AtomicCommitAllOrNothing)
{
    auto tileset = create_test_tileset("test_tileset");

    auto file1 = test_root_ / "file1.png";
    auto file2 = test_root_ / "subdir/file2.png";
    auto file3 = test_root_ / "file3.png";

    std::ofstream out1{file1};
    out1 << "original1";
    out1.close();

    auto begin_result = writer_->begin_transaction();
    ASSERT_TRUE(begin_result.has_value());

    ArtifactKey key1{"file1.png"};
    auto write_result1 = writer_->write_bottom_png(key1, tileset);
    ASSERT_TRUE(write_result1.has_value());

    ArtifactKey key2{"subdir/file2.png"};
    auto write_result2 = writer_->write_middle_png(key2, tileset);
    ASSERT_TRUE(write_result2.has_value());

    ArtifactKey key3{"file3.png"};
    auto write_result3 = writer_->write_top_png(key3, tileset);
    ASSERT_TRUE(write_result3.has_value());

    auto commit_result = writer_->commit();
    ASSERT_TRUE(commit_result.has_value());

    ASSERT_TRUE(file_contains(file1, "mock_rgba_image"));
    ASSERT_TRUE(file_contains(file2, "mock_rgba_image"));
    ASSERT_TRUE(file_contains(file3, "mock_rgba_image"));
}

TEST_F(ProjectTilesetArtifactWriterTests, TransactionSequence)
{
    auto tileset = create_test_tileset("test_tileset");

    auto begin1 = writer_->begin_transaction();
    ASSERT_TRUE(begin1.has_value());

    ArtifactKey key1{"trans1.png"};
    auto write_result1 = writer_->write_bottom_png(key1, tileset);
    ASSERT_TRUE(write_result1.has_value());

    auto commit1 = writer_->commit();
    ASSERT_TRUE(commit1.has_value());

    ASSERT_TRUE(std::filesystem::exists(test_root_ / "trans1.png"));

    auto begin2 = writer_->begin_transaction();
    ASSERT_TRUE(begin2.has_value());

    ArtifactKey key2{"trans2.png"};
    auto write_result2 = writer_->write_middle_png(key2, tileset);
    ASSERT_TRUE(write_result2.has_value());

    auto rollback2 = writer_->rollback();
    ASSERT_TRUE(rollback2.has_value());

    ASSERT_FALSE(std::filesystem::exists(test_root_ / "trans2.png"));

    auto begin3 = writer_->begin_transaction();
    ASSERT_TRUE(begin3.has_value());

    ArtifactKey key3{"trans3.png"};
    auto write_result3 = writer_->write_top_png(key3, tileset);
    ASSERT_TRUE(write_result3.has_value());

    auto commit3 = writer_->commit();
    ASSERT_TRUE(commit3.has_value());

    ASSERT_TRUE(std::filesystem::exists(test_root_ / "trans1.png"));
    ASSERT_FALSE(std::filesystem::exists(test_root_ / "trans2.png"));
    ASSERT_TRUE(std::filesystem::exists(test_root_ / "trans3.png"));
}

TEST_F(ProjectTilesetArtifactWriterTests, AttributesCsvKnobOnEmitsOneRowPerMetatileWithSynthesizedDefaults)
{
    infra_config_->write_layer_type_column = true;
    auto tileset = create_sparse_two_metatile_tileset("test_tileset");

    ASSERT_TRUE(writer_->begin_transaction().has_value());
    ArtifactKey key{"attributes.csv"};
    auto write_result = writer_->write_attributes_csv(key, tileset);
    ASSERT_TRUE(write_result.has_value()) << write_result.error().join(PlainTextFormatter{});
    ASSERT_TRUE(writer_->commit().has_value());

    // Header gains layerType. Present metatile 0 emits its "covered" token; synthesized metatile 1 emits a blank cell.
    const std::string expected = "id,behavior,layerType\n0,MB_NORMAL,covered\n1,MB_NORMAL,\n";
    EXPECT_EQ(read_whole_file(test_root_ / "attributes.csv"), expected);
}

TEST_F(ProjectTilesetArtifactWriterTests, AttributesCsvKnobOnEmitsBlankCellForInferredLayerType)
{
    // Regression: a present attribute whose layer type was set the inferred way (not pinned via explicit_layer_type)
    // must emit a BLANK layerType cell. If the writer keyed off layer_type() instead of explicit_layer_type(), it would
    // pin this row as "covered", and the next compile would wrongly treat the auto row as a user override.
    infra_config_->write_layer_type_column = true;
    auto tileset = create_two_metatile_tileset_with_inferred_layer_type("test_tileset");

    ASSERT_TRUE(writer_->begin_transaction().has_value());
    ArtifactKey key{"attributes.csv"};
    auto write_result = writer_->write_attributes_csv(key, tileset);
    ASSERT_TRUE(write_result.has_value()) << write_result.error().join(PlainTextFormatter{});
    ASSERT_TRUE(writer_->commit().has_value());

    // Both cells blank despite metatile 0's non-'normal' layer_type(): neither row carries an explicit pin.
    const std::string expected = "id,behavior,layerType\n0,MB_NORMAL,\n1,MB_NORMAL,\n";
    EXPECT_EQ(read_whole_file(test_root_ / "attributes.csv"), expected);
}

TEST_F(ProjectTilesetArtifactWriterTests, AttributesCsvKnobOffIsByteIdenticalToHistoricalOutput)
{
    // Default MockInfraConfig has write_layer_type_column = false.
    auto tileset = create_sparse_two_metatile_tileset("test_tileset");

    ASSERT_TRUE(writer_->begin_transaction().has_value());
    ArtifactKey key{"attributes.csv"};
    auto write_result = writer_->write_attributes_csv(key, tileset);
    ASSERT_TRUE(write_result.has_value()) << write_result.error().join(PlainTextFormatter{});
    ASSERT_TRUE(writer_->commit().has_value());

    // Byte-identical to the pre-schema emerald output: no layerType column, and the sole all-default metatile (1) is
    // skipped, so only metatile 0's row survives. This exact string is the emerald compatibility contract for #284.
    const std::string expected = "id,behavior\n0,MB_NORMAL\n";
    EXPECT_EQ(read_whole_file(test_root_ / "attributes.csv"), expected);
}

TEST_F(ProjectTilesetArtifactWriterTests, AttributesCsvKnobOffAllDefaultRowsWritesHeaderOnly)
{
    // Every stored attribute is all-default, so the row-omission compression collapses the file to just the header.
    auto porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    porytiles_component->bottom(Image<Rgba32>{32, 16}); // 2 metatiles
    porytiles_component->middle(Image<Rgba32>{32, 16});
    porytiles_component->top(Image<Rgba32>{32, 16});
    porytiles_component->insert_attribute(0, MetatileAttribute{});
    porytiles_component->insert_attribute(1, MetatileAttribute{});
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    porymap_component->tiles_png(create_test_indexed_image());
    Tileset tileset{"test_tileset", std::move(porytiles_component), std::move(porymap_component)};

    ASSERT_TRUE(writer_->begin_transaction().has_value());
    ArtifactKey key{"attributes.csv"};
    auto write_result = writer_->write_attributes_csv(key, tileset);
    ASSERT_TRUE(write_result.has_value()) << write_result.error().join(PlainTextFormatter{});
    ASSERT_TRUE(writer_->commit().has_value());

    EXPECT_EQ(read_whole_file(test_root_ / "attributes.csv"), "id,behavior\n");
}

// The ProviderMap membership contract: has_provider() and map membership are equivalent. A provider-backed schema
// field with no provider in the map is an internal bug, so the writer panics instead of degrading to raw rendering.
TEST_F(ProjectTilesetArtifactWriterTests, ProviderBackedFieldMissingFromProviderMapPanics)
{
    ProviderMap empty_providers{};
    ProjectTilesetArtifactWriter writer{
        domain_config_.get(),
        infra_config_.get(),
        test_root_,
        &schema_,
        &empty_providers,
        formatter_.get(),
        diag_.get(),
        png_rgba_saver_.get(),
        png_indexed_saver_.get(),
        pal_saver_.get(),
        anim_json_parser_.get(),
        anim_code_generator_.get()};

    auto tileset = create_sparse_two_metatile_tileset("test_tileset");
    ASSERT_TRUE(writer.begin_transaction().has_value());
    ArtifactKey key{"attributes.csv"};
    EXPECT_DEATH(std::ignore = writer.write_attributes_csv(key, tileset), "no provider was built");
    ASSERT_TRUE(writer.rollback().has_value());
}

// The schema-driven writer for a multi-field (stock firered shape) schema: field names become header columns,
// provider-backed cells render constant names, raw cells render plain integers, and a row is omitted only when every
// field's effective value equals its schema default.
TEST_F(ProjectTilesetArtifactWriterTests, AttributesCsvMultiFieldSchemaRendersProviderNamesAndRawIntegers)
{
    Schema firered_schema = make_firered_schema();
    ProviderMap firered_providers = make_firered_provider_map();
    ProjectTilesetArtifactWriter writer{
        domain_config_.get(),
        infra_config_.get(),
        test_root_,
        &firered_schema,
        &firered_providers,
        formatter_.get(),
        diag_.get(),
        png_rgba_saver_.get(),
        png_indexed_saver_.get(),
        pal_saver_.get(),
        anim_json_parser_.get(),
        anim_code_generator_.get()};

    auto porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    porytiles_component->bottom(Image<Rgba32>{32, 16}); // 2 metatiles
    porytiles_component->middle(Image<Rgba32>{32, 16});
    porytiles_component->top(Image<Rgba32>{32, 16});

    // Metatile 0 mixes provider-backed and raw non-defaults; metatile 1 is all-default and must be omitted.
    MetatileAttribute attr_0{};
    attr_0.field(attr::field_behavior, 2);
    attr_0.field(attr::field_terrain, 1);
    attr_0.field(attr::field_attribute_3, 5);
    attr_0.field(attr::field_encounter_type, 1);
    porytiles_component->insert_attribute(0, attr_0);
    porytiles_component->insert_attribute(1, MetatileAttribute{});

    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    porymap_component->tiles_png(create_test_indexed_image());
    Tileset tileset{"test_tileset", std::move(porytiles_component), std::move(porymap_component)};

    ASSERT_TRUE(writer.begin_transaction().has_value());
    ArtifactKey key{"attributes.csv"};
    auto write_result = writer.write_attributes_csv(key, tileset);
    ASSERT_TRUE(write_result.has_value()) << write_result.error().join(PlainTextFormatter{});
    ASSERT_TRUE(writer.commit().has_value());

    const std::string expected = "id,behavior,terrain,attribute_2,attribute_3,encounter_type,attribute_5,attribute_7\n"
                                 "0,MB_TALL_GRASS,TILE_TERRAIN_GRASS,0,5,TILE_ENCOUNTER_LAND,0,0\n";
    EXPECT_EQ(read_whole_file(test_root_ / "attributes.csv"), expected);
}

// A schema field may declare a nonzero default (the value an absent field takes). An attribute that omits the field
// renders that default, not 0. Omitting an all-default row is lossless: it reloads as an absent attribute, which the
// compiler materializes from the schema defaults, so the round trip reproduces the omitted values exactly.
TEST_F(ProjectTilesetArtifactWriterTests, AttributesCsvNonzeroDefaultRendersDefaultAndOmitsAllDefaultRows)
{
    auto schema_result = Schema::create(
        {
            Field{"behavior", 0x00FF, 0, dummy_provider_spec()},
            Field{"pad", 0x0F00, 3},
        },
        2);
    Schema schema = std::move(schema_result).value();
    ProviderMap providers{};
    providers.emplace(
        "behavior",
        std::make_unique<StubEnumMapProvider>(
            std::unordered_map<std::string, std::uint32_t>{{"MB_NORMAL", 0}, {"MB_TALL_GRASS", 2}}));
    ProjectTilesetArtifactWriter writer{
        domain_config_.get(),
        infra_config_.get(),
        test_root_,
        &schema,
        &providers,
        formatter_.get(),
        diag_.get(),
        png_rgba_saver_.get(),
        png_indexed_saver_.get(),
        pal_saver_.get(),
        anim_json_parser_.get(),
        anim_code_generator_.get()};

    auto porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    porytiles_component->bottom(Image<Rgba32>{32, 16}); // 2 metatiles
    porytiles_component->middle(Image<Rgba32>{32, 16});
    porytiles_component->top(Image<Rgba32>{32, 16});

    // Metatile 0: behavior stored, 'pad' absent -> its cell renders the default 3.
    MetatileAttribute attr_0{};
    attr_0.field(attr::field_behavior, 2);
    porytiles_component->insert_attribute(0, attr_0);

    // Metatile 1: behavior absent (effective 0 = default) and 'pad' stored as its default 3 -> all-default, so the
    // row is omitted; the compiler rematerializes it from the schema defaults on the next compile.
    MetatileAttribute attr_1{};
    attr_1.field("pad", 3);
    porytiles_component->insert_attribute(1, attr_1);

    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    porymap_component->tiles_png(create_test_indexed_image());
    Tileset tileset{"test_tileset", std::move(porytiles_component), std::move(porymap_component)};

    ASSERT_TRUE(writer.begin_transaction().has_value());
    ArtifactKey key{"attributes.csv"};
    auto write_result = writer.write_attributes_csv(key, tileset);
    ASSERT_TRUE(write_result.has_value()) << write_result.error().join(PlainTextFormatter{});
    ASSERT_TRUE(writer.commit().has_value());

    const std::string expected = "id,behavior,pad\n0,MB_TALL_GRASS,3\n";
    EXPECT_EQ(read_whole_file(test_root_ / "attributes.csv"), expected);
}