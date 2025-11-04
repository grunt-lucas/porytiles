#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "porytiles2/domain/models/image.hpp"
#include "porytiles2/domain/models/index_pixel.hpp"
#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/porymap_tileset_component.hpp"
#include "porytiles2/domain/models/porytiles_tileset_component.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tilemap_entry.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/infra/config/infra_config.hpp"
#include "porytiles2/infra/repos/project_tileset_artifact_writer.hpp"
#include "porytiles2/infra/services/file_pal_saver.hpp"
#include "porytiles2/infra/services/png_indexed_image_saver.hpp"
#include "porytiles2/infra/services/png_rgba_image_saver.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/config/config_value.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

using namespace porytiles2;

namespace {

class MockInfraConfig : public InfraConfig {
  public:
    [[nodiscard]] ChainableResult<ConfigValue<TilesPalMode>> tiles_pal_mode(const std::string &) const override
    {
        return ConfigValue<TilesPalMode>{TilesPalMode::true_color, "tiles_pal_mode", "mock", {}};
    }

    void test_root(const std::filesystem::path &path)
    {
        test_root_ = path;
    }

    [[nodiscard]] const std::filesystem::path &get_test_root() const
    {
        return test_root_;
    }

  private:
    std::filesystem::path test_root_;
};

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
    save(const Palette<Rgba32> &pal, const std::filesystem::path &path) const override
    {
        std::ofstream out{path};
        out << "mock_palette";
        return {};
    }
};

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
        porymap_component->set_pal(Palette<Rgba32>{}, i);
    }

    // TODO: this test is flaky, once our tileset reader/writer account for num_tiles_per_metatile, we'll need to come
    // back and update this test. For now, let's just assume we have dual-layer metatiles so there should be 8
    // attributes
    for (int i = 0; i < 8; i++) {
        porymap_component->push_back_attribute(MetatileAttribute{});
    }

    return Tileset{name, std::move(porytiles_component), std::move(porymap_component)};
}

} // namespace

class ProjectTilesetArtifactWriterTests : public ::testing::Test {
  protected:
    void SetUp() override
    {
        config_ = std::make_unique<MockInfraConfig>();
        png_rgba_saver_ = std::make_unique<MockPngRgbaImageSaver>();
        png_indexed_saver_ = std::make_unique<MockPngIndexedImageSaver>();
        pal_saver_ = std::make_unique<MockFilePalSaver>();

        test_root_ = std::filesystem::temp_directory_path() / "porytiles_artifact_writer_tests";
        std::filesystem::create_directories(test_root_);
        config_->test_root(test_root_);

        writer_ = std::make_unique<ProjectTilesetArtifactWriter>(
            config_.get(), test_root_, png_rgba_saver_.get(), png_indexed_saver_.get(), pal_saver_.get());
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
        auto tmp_dir = std::filesystem::temp_directory_path();
        for (const auto &entry : std::filesystem::directory_iterator(tmp_dir)) {
            if (entry.is_directory() && entry.path().filename().string().starts_with("porytiles_")) {
                count++;
            }
        }
        return count;
    }

    std::filesystem::path test_root_;
    std::unique_ptr<MockInfraConfig> config_;
    std::unique_ptr<MockPngRgbaImageSaver> png_rgba_saver_;
    std::unique_ptr<MockPngIndexedImageSaver> png_indexed_saver_;
    std::unique_ptr<MockFilePalSaver> pal_saver_;
    std::unique_ptr<ProjectTilesetArtifactWriter> writer_;
};

TEST_F(ProjectTilesetArtifactWriterTests, BasicTransactionLifecycle)
{
    auto tileset = create_test_tileset("test_tileset");

    auto begin_result = writer_->begin_transaction();
    ASSERT_TRUE(begin_result.has_value());

    auto expected_file = test_root_ / "tileset_bottom.png";
    ArtifactKey key{expected_file.string()};
    TilesetArtifact artifact{TilesetArtifact::Type::bottom_png};
    auto write_result = writer_->write(key, artifact, tileset);
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
    ArtifactKey key{expected_file.string()};
    TilesetArtifact artifact{TilesetArtifact::Type::bottom_png};
    auto write_result = writer_->write(key, artifact, tileset);
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

    ArtifactKey key1{(test_root_ / "assets/bottom.png").string()};
    TilesetArtifact artifact1{TilesetArtifact::Type::bottom_png};
    auto write_result1 = writer_->write(key1, artifact1, tileset);
    ASSERT_TRUE(write_result1.has_value());

    ArtifactKey key2{(test_root_ / "assets/middle.png").string()};
    TilesetArtifact artifact2{TilesetArtifact::Type::middle_png};
    auto write_result2 = writer_->write(key2, artifact2, tileset);
    ASSERT_TRUE(write_result2.has_value());

    ArtifactKey key3{(test_root_ / "assets/tiles.png").string()};
    TilesetArtifact artifact3{TilesetArtifact::Type::tiles_png};
    auto write_result3 = writer_->write(key3, artifact3, tileset);
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

    ArtifactKey key{existing_file.string()};
    TilesetArtifact artifact{TilesetArtifact::Type::bottom_png};
    auto write_result = writer_->write(key, artifact, tileset);
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
    ArtifactKey key{expected_file.string()};
    TilesetArtifact artifact{TilesetArtifact::Type::top_png};
    auto write_result = writer_->write(key, artifact, tileset);
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
    EXPECT_EQ(commit_error_lines[0], "no transaction in progress");

    auto rollback_result = writer_->rollback();
    ASSERT_FALSE(rollback_result.has_value());
    EXPECT_EQ(rollback_result.error(), "no transaction in progress");

    ArtifactKey key{(test_root_ / "no_transaction.png").string()};
    TilesetArtifact artifact{TilesetArtifact::Type::bottom_png};
    auto write_result = writer_->write(key, artifact, tileset);
    ASSERT_FALSE(write_result.has_value());
    auto error_lines = write_result.error().details(PlainTextFormatter{});
    ASSERT_EQ(error_lines.size(), 1);
    EXPECT_EQ(error_lines[0], "no transaction in progress");
}

TEST_F(ProjectTilesetArtifactWriterTests, DoubleBeginTransaction)
{
    auto begin_result1 = writer_->begin_transaction();
    ASSERT_TRUE(begin_result1.has_value());

    auto begin_result2 = writer_->begin_transaction();
    ASSERT_FALSE(begin_result2.has_value());
    EXPECT_EQ(begin_result2.error(), "transaction already in progress");

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

    ArtifactKey key{(test_root_ / "temp_cleanup.png").string()};
    TilesetArtifact artifact{TilesetArtifact::Type::bottom_png};
    auto write_result = writer_->write(key, artifact, tileset);
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
    ArtifactKey key{expected_file.string()};
    TilesetArtifact artifact{TilesetArtifact::Type::metatiles_bin};
    auto write_result = writer_->write(key, artifact, tileset);
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
    ArtifactKey key{expected_file.string()};
    TilesetArtifact artifact{TilesetArtifact::Type::metatile_attributes_bin};
    auto write_result = writer_->write(key, artifact, tileset);
    ASSERT_TRUE(write_result.has_value());

    auto commit_result = writer_->commit();
    ASSERT_TRUE(commit_result.has_value());
    ASSERT_TRUE(std::filesystem::exists(expected_file));
    // we wrote 8 attributes in create_test_tileset, so size should be 16 bytes
    // TODO: we'll want to fix this later once we properly handle num_tiles_per_metatile
    ASSERT_EQ(std::filesystem::file_size(expected_file), 16);
}

TEST_F(ProjectTilesetArtifactWriterTests, WritePalette)
{
    auto tileset = create_test_tileset("test_tileset");

    auto begin_result = writer_->begin_transaction();
    ASSERT_TRUE(begin_result.has_value());

    auto expected_file = test_root_ / "palette_0.pal";
    ArtifactKey key{expected_file.string()};
    TilesetArtifact artifact{TilesetArtifact::Type::pal_n, 0};
    auto write_result = writer_->write(key, artifact, tileset);
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

    ArtifactKey key1{file1.string()};
    TilesetArtifact artifact1{TilesetArtifact::Type::bottom_png};
    auto write_result1 = writer_->write(key1, artifact1, tileset);
    ASSERT_TRUE(write_result1.has_value());

    ArtifactKey key2{file2.string()};
    TilesetArtifact artifact2{TilesetArtifact::Type::middle_png};
    auto write_result2 = writer_->write(key2, artifact2, tileset);
    ASSERT_TRUE(write_result2.has_value());

    ArtifactKey key3{file3.string()};
    TilesetArtifact artifact3{TilesetArtifact::Type::top_png};
    auto write_result3 = writer_->write(key3, artifact3, tileset);
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

    ArtifactKey key1{(test_root_ / "trans1.png").string()};
    TilesetArtifact artifact1{TilesetArtifact::Type::bottom_png};
    auto write_result1 = writer_->write(key1, artifact1, tileset);
    ASSERT_TRUE(write_result1.has_value());

    auto commit1 = writer_->commit();
    ASSERT_TRUE(commit1.has_value());

    ASSERT_TRUE(std::filesystem::exists(test_root_ / "trans1.png"));

    auto begin2 = writer_->begin_transaction();
    ASSERT_TRUE(begin2.has_value());

    ArtifactKey key2{(test_root_ / "trans2.png").string()};
    TilesetArtifact artifact2{TilesetArtifact::Type::middle_png};
    auto write_result2 = writer_->write(key2, artifact2, tileset);
    ASSERT_TRUE(write_result2.has_value());

    auto rollback2 = writer_->rollback();
    ASSERT_TRUE(rollback2.has_value());

    ASSERT_FALSE(std::filesystem::exists(test_root_ / "trans2.png"));

    auto begin3 = writer_->begin_transaction();
    ASSERT_TRUE(begin3.has_value());

    ArtifactKey key3{(test_root_ / "trans3.png").string()};
    TilesetArtifact artifact3{TilesetArtifact::Type::top_png};
    auto write_result3 = writer_->write(key3, artifact3, tileset);
    ASSERT_TRUE(write_result3.has_value());

    auto commit3 = writer_->commit();
    ASSERT_TRUE(commit3.has_value());

    ASSERT_TRUE(std::filesystem::exists(test_root_ / "trans1.png"));
    ASSERT_FALSE(std::filesystem::exists(test_root_ / "trans2.png"));
    ASSERT_TRUE(std::filesystem::exists(test_root_ / "trans3.png"));
}