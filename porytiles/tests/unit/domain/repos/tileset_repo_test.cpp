#include "gtest/gtest.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/porymap_tileset_component.hpp"
#include "porytiles/domain/models/porytiles_tileset_component.hpp"
#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/domain/repos/artifact_checksum_provider.hpp"
#include "porytiles/domain/repos/artifact_key.hpp"
#include "porytiles/domain/repos/tileset_artifact_reader.hpp"
#include "porytiles/domain/repos/tileset_artifact_writer.hpp"
#include "porytiles/domain/repos/tileset_repo.hpp"
#include "porytiles/domain/services/tileset_metadata_provider.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/string_utils.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

#include "support/fake_tileset_artifact_key_provider.hpp"

using namespace porytiles;

namespace {

class StubArtifactChecksumProvider final : public ArtifactChecksumProvider {
  public:
    [[nodiscard]] std::unordered_map<ArtifactKey, std::string>
    compute_tileset_artifact_checksums(const std::vector<ArtifactKey> &) const override
    {
        return {};
    }

    [[nodiscard]] std::unordered_map<ArtifactKey, std::string>
    load_cached_tileset_checksums(const std::string &) const override
    {
        return {};
    }

    [[nodiscard]] ChainableResult<void>
    cache_tileset_checksums(const std::string &, const std::unordered_map<ArtifactKey, std::string> &) const override
    {
        return {};
    }
};

class StubTilesetMetadataProvider final : public TilesetMetadataProvider {
  public:
    [[nodiscard]] bool exists(const std::string &) const override
    {
        return true;
    }

    [[nodiscard]] ChainableResult<bool> is_secondary(const std::string &) const override
    {
        return false;
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

// Every read succeeds and leaves the tileset untouched: these tests exercise which artifacts TilesetRepo asks for,
// not how they parse.
class StubTilesetArtifactReader final : public TilesetArtifactReader {
  public:
    [[nodiscard]] ChainableResult<void> read_metatiles_bin(Tileset &, const ArtifactKey &) const override
    {
        return {};
    }

    [[nodiscard]] ChainableResult<void> read_metatile_attributes_bin(Tileset &, const ArtifactKey &) const override
    {
        return {};
    }

    [[nodiscard]] ChainableResult<void> read_tiles_png(Tileset &, const ArtifactKey &) const override
    {
        return {};
    }

    [[nodiscard]] ChainableResult<void>
    read_porymap_palette_n(Tileset &, const ArtifactKey &, std::size_t) const override
    {
        return {};
    }

    [[nodiscard]] ChainableResult<void> read_porymap_anim(
        Tileset &,
        const std::string &,
        const ArtifactKey &,
        const std::vector<std::pair<std::string, ArtifactKey>> &) const override
    {
        return {};
    }

    [[nodiscard]] ChainableResult<void> read_bottom_png(Tileset &, const ArtifactKey &) const override
    {
        return {};
    }

    [[nodiscard]] ChainableResult<void> read_middle_png(Tileset &, const ArtifactKey &) const override
    {
        return {};
    }

    [[nodiscard]] ChainableResult<void> read_top_png(Tileset &, const ArtifactKey &) const override
    {
        return {};
    }

    [[nodiscard]] ChainableResult<void> read_attributes_csv(Tileset &, const ArtifactKey &) const override
    {
        return {};
    }

    [[nodiscard]] ChainableResult<void>
    read_porytiles_palette_n(Tileset &, const ArtifactKey &, std::size_t) const override
    {
        return {};
    }

    [[nodiscard]] ChainableResult<void> read_porytiles_anim(
        Tileset &,
        const std::string &,
        const ArtifactKey &,
        const std::optional<ArtifactKey> &,
        const std::vector<std::pair<std::string, ArtifactKey>> &) const override
    {
        return {};
    }

    [[nodiscard]] ChainableResult<void>
    read_porytiles_primary_anim_references(Tileset &, const ArtifactKey &) const override
    {
        return {};
    }
};

// Records the order of the transaction calls TilesetRepo makes. Entries read "begin", "commit", "rollback",
// "remove:<key>", and "write:<key>".
class RecordingTilesetArtifactWriter final : public TilesetArtifactWriter {
  public:
    std::vector<std::string> calls{};
    bool fail_removals = false;

    [[nodiscard]] ChainableResult<void> begin_transaction() override
    {
        calls.emplace_back("begin");
        return {};
    }

    [[nodiscard]] ChainableResult<void> commit() override
    {
        calls.emplace_back("commit");
        return {};
    }

    [[nodiscard]] ChainableResult<void> rollback() override
    {
        calls.emplace_back("rollback");
        return {};
    }

    [[nodiscard]] ChainableResult<void> remove_artifact(const ArtifactKey &key) override
    {
        if (fail_removals) {
            return FormattableError{"Removal refused."};
        }
        calls.emplace_back("remove:" + key.key());
        return {};
    }

    [[nodiscard]] ChainableResult<void> write_metatiles_bin(const ArtifactKey &dest_key, const Tileset &) override
    {
        return record_write(dest_key);
    }

    [[nodiscard]] ChainableResult<void>
    write_metatile_attributes_bin(const ArtifactKey &dest_key, const Tileset &) override
    {
        return record_write(dest_key);
    }

    [[nodiscard]] ChainableResult<void> write_tiles_png(const ArtifactKey &dest_key, const Tileset &) override
    {
        return record_write(dest_key);
    }

    [[nodiscard]] ChainableResult<void>
    write_porymap_palette_n(const ArtifactKey &dest_key, const Tileset &, std::size_t) override
    {
        return record_write(dest_key);
    }

    [[nodiscard]] ChainableResult<void> write_porymap_anim_frame(
        const ArtifactKey &dest_key, const Tileset &, const std::string &, const std::string &) override
    {
        return record_write(dest_key);
    }

    [[nodiscard]] ChainableResult<void> write_porymap_anim_params(const ArtifactKey &dest_key, const Tileset &) override
    {
        return record_write(dest_key);
    }

    [[nodiscard]] ChainableResult<void> write_bottom_png(const ArtifactKey &dest_key, const Tileset &) override
    {
        return record_write(dest_key);
    }

    [[nodiscard]] ChainableResult<void> write_middle_png(const ArtifactKey &dest_key, const Tileset &) override
    {
        return record_write(dest_key);
    }

    [[nodiscard]] ChainableResult<void> write_top_png(const ArtifactKey &dest_key, const Tileset &) override
    {
        return record_write(dest_key);
    }

    [[nodiscard]] ChainableResult<void> write_attributes_csv(const ArtifactKey &dest_key, const Tileset &) override
    {
        return record_write(dest_key);
    }

    [[nodiscard]] ChainableResult<void>
    write_porytiles_palette_n(const ArtifactKey &dest_key, const Tileset &, std::size_t) override
    {
        return record_write(dest_key);
    }

    [[nodiscard]] ChainableResult<void> write_porytiles_anim_frame(
        const ArtifactKey &dest_key, const Tileset &, const std::string &, const std::string &) override
    {
        return record_write(dest_key);
    }

    [[nodiscard]] ChainableResult<void>
    write_porytiles_anim_params(const ArtifactKey &dest_key, const Tileset &) override
    {
        return record_write(dest_key);
    }

  private:
    [[nodiscard]] ChainableResult<void> record_write(const ArtifactKey &dest_key)
    {
        calls.emplace_back("write:" + dest_key.key());
        return {};
    }
};

// The Porymap artifacts TilesetRepo::load() refuses to continue without.
std::set<ArtifactKey> required_porymap_keys()
{
    std::set<ArtifactKey> keys{
        ArtifactKey{"bin/metatiles.bin"}, ArtifactKey{"bin/metatile_attributes.bin"}, ArtifactKey{"bin/tiles.png"}};
    for (std::size_t i = 0; i < palette::num_palettes; i++) {
        keys.insert(ArtifactKey{"bin/palettes/" + palette_filename(i)});
    }
    return keys;
}

Tileset make_empty_tileset(const std::string &name)
{
    return Tileset{name, std::make_unique<PorytilesTilesetComponent>(), std::make_unique<PorymapTilesetComponent>()};
}

} // namespace

class TilesetRepoTests : public ::testing::Test {
  protected:
    StubArtifactChecksumProvider checksum_provider_{};
    StubTilesetMetadataProvider metadata_provider_{};
    FakeTilesetArtifactKeyProvider key_provider_{};
    StubTilesetArtifactReader reader_{};
    RecordingTilesetArtifactWriter writer_{};
    BufferedUserDiagnostics diag_{};
    TilesetRepo repo_{&checksum_provider_, &metadata_provider_, &key_provider_, &reader_, &writer_, &diag_};
};

TEST_F(TilesetRepoTests, LoadAnimsWithoutAnimParams)
{
    key_provider_.existing = required_porymap_keys();
    key_provider_.porytiles_anims = {{"flower", {"00"}}, {"water", {"00", "01"}}};

    auto result = repo_.load("test_tileset");

    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(diag_.errors().size(), 1);
    EXPECT_EQ(diag_.errors().at(0), std::vector<std::string>{"Missing required artifact: 'src/anim/anim.json'."});
    ASSERT_EQ(diag_.error_notes().size(), 1);
    EXPECT_EQ(
        diag_.error_notes().at(0),
        std::vector<std::string>{"Animations found without animation parameters: 'flower', 'water'."});
}

// An animation with no frames has nothing the missing parameters could cause to be left out, so it is not an error.
// This is the state a save leaves behind when it removes an animation whose location also holds unrelated files.
TEST_F(TilesetRepoTests, LoadFramelessAnimsWithoutAnimParams)
{
    key_provider_.existing = required_porymap_keys();
    key_provider_.porytiles_anims = {{"water", {}}};

    auto result = repo_.load("test_tileset");

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(diag_.errors().empty());
}

TEST_F(TilesetRepoTests, LoadWithoutAnimsOrAnimParams)
{
    key_provider_.existing = required_porymap_keys();

    auto result = repo_.load("test_tileset");

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(diag_.errors().empty());
}

TEST_F(TilesetRepoTests, LoadAnimsWithAnimParams)
{
    key_provider_.existing = required_porymap_keys();
    key_provider_.existing.insert(ArtifactKey{"src/anim/anim.json"});
    key_provider_.existing.insert(ArtifactKey{"src/anim/water/00.png"});
    key_provider_.porytiles_anims = {{"water", {"00"}}};

    auto result = repo_.load("test_tileset");

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(diag_.errors().empty());
}

// The saved tileset has no animations, so the animation artifacts already in the backing store are obsolete. The repo
// must mark every pre-existing artifact for removal before it writes anything: a write takes precedence over the
// removal of its own key, so the writer ends up removing exactly the artifacts the tileset no longer has.
TEST_F(TilesetRepoTests, SaveRetiresExistingArtifacts)
{
    key_provider_.existing = {
        ArtifactKey{"bin/metatiles.bin"},
        ArtifactKey{"bin/anim/water/00.png"},
        ArtifactKey{"generated/anim_code.h"},
        ArtifactKey{"src/bottom.png"},
        ArtifactKey{"src/palettes/03.pal"},
        ArtifactKey{"src/anim/anim.json"},
        ArtifactKey{"src/anim/water/00.png"},
    };
    key_provider_.porymap_anims = {{"water", {"00"}}};
    key_provider_.porytiles_anims = {{"water", {"00"}}};

    auto result = repo_.save(make_empty_tileset("test_tileset"));

    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(writer_.calls.empty());
    EXPECT_EQ(writer_.calls.front(), "begin");
    EXPECT_EQ(writer_.calls.back(), "commit");

    const auto is_removal = [](const std::string &call) { return call.starts_with("remove:"); };
    const auto is_write = [](const std::string &call) { return call.starts_with("write:"); };
    const auto first_write = std::ranges::find_if(writer_.calls, is_write);
    ASSERT_NE(first_write, writer_.calls.end());
    EXPECT_TRUE(std::none_of(first_write, writer_.calls.end(), is_removal));

    std::set<std::string> removed{};
    for (const auto &call : writer_.calls) {
        if (is_removal(call)) {
            removed.insert(call);
        }
    }
    const std::set<std::string> expected{
        "remove:bin/metatiles.bin",
        "remove:bin/anim/water/00.png",
        "remove:generated/anim_code.h",
        "remove:src/bottom.png",
        "remove:src/palettes/03.pal",
        "remove:src/anim/anim.json",
        "remove:src/anim/water/00.png",
    };
    EXPECT_EQ(removed, expected);
}

TEST_F(TilesetRepoTests, SaveRemovalFailure)
{
    key_provider_.existing = {ArtifactKey{"src/bottom.png"}};
    writer_.fail_removals = true;

    auto result = repo_.save(make_empty_tileset("test_tileset"));

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(writer_.calls, (std::vector<std::string>{"begin", "rollback"}));
}
