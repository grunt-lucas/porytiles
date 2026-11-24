#include "gtest/gtest.h"

#include <vector>

#include "porytiles/domain/repos/artifact_key.hpp"
#include "porytiles/domain/repos/tileset_artifact_key_provider.hpp"

#include "support/fake_tileset_artifact_key_provider.hpp"

using namespace porytiles;

TEST(TilesetArtifactKeyProviderTests, PorytilesArtifactKeysWithAnimParams)
{
    FakeTilesetArtifactKeyProvider provider{};
    provider.existing = {
        ArtifactKey{"src/bottom.png"},
        ArtifactKey{"src/anim/anim.json"},
        ArtifactKey{"src/anim/water/key.png"},
        ArtifactKey{"src/anim/water/00.png"},
    };
    provider.porytiles_anims = {{"water", {"key", "00"}}};

    auto result = provider.get_porytiles_artifact_keys("test_tileset");

    ASSERT_TRUE(result.has_value());
    const std::vector<ArtifactKey> expected{
        ArtifactKey{"src/bottom.png"},
        ArtifactKey{"src/anim/water/00.png"},
        ArtifactKey{"src/anim/water/key.png"},
        ArtifactKey{"src/anim/anim.json"},
    };
    EXPECT_EQ(result.value(), expected);
}

// The animation parameters decide which animations a tileset has, so frames found without them are not artifacts of
// the tileset. TilesetRepo::save() retires every key this method returns, which makes the exclusion the reason a save
// leaves such frames alone.
TEST(TilesetArtifactKeyProviderTests, PorytilesArtifactKeysWithoutAnimParams)
{
    FakeTilesetArtifactKeyProvider provider{};
    provider.existing = {
        ArtifactKey{"src/bottom.png"},
        ArtifactKey{"src/anim/water/key.png"},
        ArtifactKey{"src/anim/water/00.png"},
    };
    provider.porytiles_anims = {{"water", {"key", "00"}}};

    auto result = provider.get_porytiles_artifact_keys("test_tileset");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), std::vector<ArtifactKey>{ArtifactKey{"src/bottom.png"}});
}
