#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "porytiles/domain/repos/artifact_key.hpp"
#include "porytiles/domain/services/tileset_metadata_provider.hpp"
#include "porytiles/infra/repos/project_tileset_artifact_key_provider.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

#include "support/mock_infra_config.hpp"

using namespace porytiles;

namespace {

class StubTilesetMetadataProvider final : public TilesetMetadataProvider {
  public:
    bool secondary = false;
    bool fail_is_secondary = false;

    [[nodiscard]] bool exists(const std::string &) const override
    {
        return true;
    }

    [[nodiscard]] ChainableResult<bool> is_secondary(const std::string &) const override
    {
        if (fail_is_secondary) {
            return FormattableError{"Metadata unavailable."};
        }
        return secondary;
    }

    [[nodiscard]] ChainableResult<bool> has_animations(const std::string &) const override
    {
        return false;
    }

    [[nodiscard]] ChainableResult<std::set<std::string>> tilesets() const override
    {
        return std::set<std::string>{};
    }
};

void write_text_file(const std::filesystem::path &path, const std::string &content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out{path};
    out << content;
}

// The messages of an error chain from outermost to root cause, leaving out the links that carry no message.
template <typename T>
[[nodiscard]] std::vector<std::string> chain_messages(const ChainableResult<T> &result)
{
    std::vector<std::string> messages{};
    for (const auto &error : result.chain()) {
        const auto details = error->details(PlainTextFormatter{});
        if (!details.empty()) {
            messages.push_back(details.at(0));
        }
    }
    return messages;
}

} // namespace

// Every test configures four distinct directories, so a key built from the wrong one is visible in the key itself.
class ProjectTilesetArtifactKeyProviderTests : public ::testing::Test {
  protected:
    void SetUp() override
    {
        infra_config_.tileset_paths_primary_src = "src_assets/primary";
        infra_config_.tileset_paths_primary_bin = "bin_assets/primary";
        infra_config_.tileset_paths_secondary_src = "src_assets/secondary";
        infra_config_.tileset_paths_secondary_bin = "bin_assets/secondary";

        test_root_ = std::filesystem::temp_directory_path() / "porytiles_artifact_key_provider_tests";
        std::filesystem::remove_all(test_root_);
        std::filesystem::create_directories(test_root_);

        provider_ = std::make_unique<ProjectTilesetArtifactKeyProvider>(
            test_root_, &infra_config_, &metadata_provider_, &formatter_, &diag_);
    }

    void TearDown() override
    {
        std::filesystem::remove_all(test_root_);
    }

    [[nodiscard]] static std::string key_of(const ChainableResult<ArtifactKey> &result)
    {
        return result.has_value() ? result.value().key() : result.error().join(PlainTextFormatter{});
    }

    const std::string tileset_name_ = "gTileset_TestTileset";
    std::filesystem::path test_root_;
    PlainTextFormatter formatter_{};
    BufferedUserDiagnostics diag_{};
    MockInfraConfig infra_config_{};
    StubTilesetMetadataProvider metadata_provider_{};
    std::unique_ptr<ProjectTilesetArtifactKeyProvider> provider_;
};

TEST_F(ProjectTilesetArtifactKeyProviderTests, PrimaryPorytilesKeys)
{
    const std::string src_dir = "src_assets/primary/test_tileset/porytiles_src/";

    EXPECT_EQ(key_of(provider_->key_for_bottom_png(tileset_name_)), src_dir + "bottom.png");
    EXPECT_EQ(key_of(provider_->key_for_middle_png(tileset_name_)), src_dir + "middle.png");
    EXPECT_EQ(key_of(provider_->key_for_top_png(tileset_name_)), src_dir + "top.png");
    EXPECT_EQ(key_of(provider_->key_for_attributes_csv(tileset_name_)), src_dir + "attributes.csv");
    EXPECT_EQ(key_of(provider_->key_for_porytiles_palette_n(tileset_name_, 3)), src_dir + "palettes/03.pal");
    EXPECT_EQ(
        key_of(provider_->key_for_porytiles_anim_frame(tileset_name_, "water", "00")), src_dir + "anim/water/00.png");
    EXPECT_EQ(key_of(provider_->key_for_porytiles_anim_params(tileset_name_)), src_dir + "anim/anim.json");
}

TEST_F(ProjectTilesetArtifactKeyProviderTests, SecondaryPorytilesKeys)
{
    metadata_provider_.secondary = true;
    const std::string src_dir = "src_assets/secondary/test_tileset/porytiles_src/";

    EXPECT_EQ(key_of(provider_->key_for_bottom_png(tileset_name_)), src_dir + "bottom.png");
    EXPECT_EQ(key_of(provider_->key_for_middle_png(tileset_name_)), src_dir + "middle.png");
    EXPECT_EQ(key_of(provider_->key_for_top_png(tileset_name_)), src_dir + "top.png");
    EXPECT_EQ(key_of(provider_->key_for_attributes_csv(tileset_name_)), src_dir + "attributes.csv");
    EXPECT_EQ(key_of(provider_->key_for_porytiles_palette_n(tileset_name_, 3)), src_dir + "palettes/03.pal");
    EXPECT_EQ(
        key_of(provider_->key_for_porytiles_anim_frame(tileset_name_, "water", "00")), src_dir + "anim/water/00.png");
    EXPECT_EQ(key_of(provider_->key_for_porytiles_anim_params(tileset_name_)), src_dir + "anim/anim.json");
}

TEST_F(ProjectTilesetArtifactKeyProviderTests, PrimaryPorymapKeys)
{
    const std::string bin_dir = "bin_assets/primary/test_tileset/porytiles_bin/";

    EXPECT_EQ(key_of(provider_->key_for_metatiles_bin(tileset_name_)), bin_dir + "metatiles.bin");
    EXPECT_EQ(key_of(provider_->key_for_metatile_attributes_bin(tileset_name_)), bin_dir + "metatile_attributes.bin");
    EXPECT_EQ(key_of(provider_->key_for_tiles_png(tileset_name_)), bin_dir + "tiles.png");
    EXPECT_EQ(key_of(provider_->key_for_porymap_palette_n(tileset_name_, 3)), bin_dir + "palettes/03.pal");
    EXPECT_EQ(
        key_of(provider_->key_for_porymap_anim_frame(tileset_name_, "water", "00")), bin_dir + "anim/water/00.png");
}

// The generated animation code lives outside both asset directories, at a fixed location under include/.
TEST_F(ProjectTilesetArtifactKeyProviderTests, PorymapAnimParamsKey)
{
    EXPECT_EQ(
        key_of(provider_->key_for_porymap_anim_params(tileset_name_)),
        "include/porytiles_generated/tilesets/test_tileset/generated_anim_code.h");
}

TEST_F(ProjectTilesetArtifactKeyProviderTests, SecondaryPorymapKeys)
{
    metadata_provider_.secondary = true;
    const std::string bin_dir = "bin_assets/secondary/test_tileset/porytiles_bin/";

    EXPECT_EQ(key_of(provider_->key_for_metatiles_bin(tileset_name_)), bin_dir + "metatiles.bin");
    EXPECT_EQ(key_of(provider_->key_for_metatile_attributes_bin(tileset_name_)), bin_dir + "metatile_attributes.bin");
    EXPECT_EQ(key_of(provider_->key_for_tiles_png(tileset_name_)), bin_dir + "tiles.png");
    EXPECT_EQ(key_of(provider_->key_for_porymap_palette_n(tileset_name_, 3)), bin_dir + "palettes/03.pal");
    EXPECT_EQ(
        key_of(provider_->key_for_porymap_anim_frame(tileset_name_, "water", "00")), bin_dir + "anim/water/00.png");
}

// Discovery scans the filesystem and the key methods build paths. Both must point at the same directory, or a frame
// that discovery reports has a key that does not exist.
TEST_F(ProjectTilesetArtifactKeyProviderTests, DiscoveredPorytilesAnimFramesHaveExistingKeys)
{
    const auto src_dir = test_root_ / "src_assets/primary/test_tileset/porytiles_src";
    write_text_file(src_dir / "bottom.png", "bottom");
    write_text_file(src_dir / "anim/anim.json", "{}");
    write_text_file(src_dir / "anim/water/key.png", "key");
    write_text_file(src_dir / "anim/water/00.png", "frame");

    auto anims = provider_->discover_porytiles_anims(tileset_name_);
    ASSERT_TRUE(anims.has_value());
    EXPECT_EQ(anims.value(), std::set<std::string>{"water"});

    auto frames = provider_->discover_porytiles_anim_frames(tileset_name_, "water");
    ASSERT_TRUE(frames.has_value());
    EXPECT_EQ(frames.value(), (std::set<std::string>{"00", "key"}));

    for (const auto &frame : frames.value()) {
        auto frame_key = provider_->key_for_porytiles_anim_frame(tileset_name_, "water", frame);
        ASSERT_TRUE(frame_key.has_value());
        EXPECT_TRUE(provider_->artifact_exists(frame_key.value())) << frame_key.value().key();
    }

    auto keys = provider_->get_porytiles_artifact_keys(tileset_name_);
    ASSERT_TRUE(keys.has_value());
    const std::string key_dir = "src_assets/primary/test_tileset/porytiles_src/";
    const std::vector<ArtifactKey> expected{
        ArtifactKey{key_dir + "bottom.png"},
        ArtifactKey{key_dir + "anim/water/00.png"},
        ArtifactKey{key_dir + "anim/water/key.png"},
        ArtifactKey{key_dir + "anim/anim.json"},
    };
    EXPECT_EQ(keys.value(), expected);
}

TEST_F(ProjectTilesetArtifactKeyProviderTests, DiscoveredPorymapAnimFramesHaveExistingKeys)
{
    const auto bin_dir = test_root_ / "bin_assets/primary/test_tileset/porytiles_bin";
    write_text_file(bin_dir / "anim/water/00.png", "frame");

    auto anims = provider_->discover_porymap_anims(tileset_name_);
    ASSERT_TRUE(anims.has_value());
    EXPECT_EQ(anims.value(), std::set<std::string>{"water"});

    auto frames = provider_->discover_porymap_anim_frames(tileset_name_, "water");
    ASSERT_TRUE(frames.has_value());
    EXPECT_EQ(frames.value(), std::set<std::string>{"00"});

    auto frame_key = provider_->key_for_porymap_anim_frame(tileset_name_, "water", "00");
    ASSERT_TRUE(frame_key.has_value());
    EXPECT_TRUE(provider_->artifact_exists(frame_key.value())) << frame_key.value().key();
}

// The asset directory cannot be resolved without the tileset's primary/secondary status, so both the key methods and
// the discovery methods report the metadata failure.
TEST_F(ProjectTilesetArtifactKeyProviderTests, MetadataFailure)
{
    metadata_provider_.fail_is_secondary = true;
    const std::vector<std::string> expected{"Failed to determine primary/secondary status.", "Metadata unavailable."};

    auto porytiles_key = provider_->key_for_bottom_png(tileset_name_);
    ASSERT_FALSE(porytiles_key.has_value());
    EXPECT_EQ(chain_messages(porytiles_key), expected);

    auto porymap_key = provider_->key_for_metatiles_bin(tileset_name_);
    ASSERT_FALSE(porymap_key.has_value());
    EXPECT_EQ(chain_messages(porymap_key), expected);

    auto porytiles_anims = provider_->discover_porytiles_anims(tileset_name_);
    ASSERT_FALSE(porytiles_anims.has_value());
    EXPECT_EQ(chain_messages(porytiles_anims), expected);

    auto porymap_frames = provider_->discover_porymap_anim_frames(tileset_name_, "water");
    ASSERT_FALSE(porymap_frames.has_value());
    EXPECT_EQ(chain_messages(porymap_frames), expected);
}
