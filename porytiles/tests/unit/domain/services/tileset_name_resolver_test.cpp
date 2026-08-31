#include "gtest/gtest.h"

#include <set>
#include <string>

#include "porytiles/domain/services/tileset_name_resolver.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"

using namespace porytiles;

class TilesetNameResolverTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_{};
    std::set<std::string> tilesets_{"gTileset_General", "gTileset_PetalburgCity", "gTileset_SecretBase"};
};

TEST_F(TilesetNameResolverTest, ExactName)
{
    auto result = resolve_tileset_name("gTileset_SecretBase", tilesets_, &formatter_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "gTileset_SecretBase");
}

TEST_F(TilesetNameResolverTest, PascalCaseShorthand)
{
    auto result = resolve_tileset_name("SecretBase", tilesets_, &formatter_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "gTileset_SecretBase");
}

TEST_F(TilesetNameResolverTest, SnakeCaseShorthand)
{
    auto result = resolve_tileset_name("secret_base", tilesets_, &formatter_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "gTileset_SecretBase");
}

TEST_F(TilesetNameResolverTest, CamelCaseShorthand)
{
    auto result = resolve_tileset_name("secretBase", tilesets_, &formatter_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "gTileset_SecretBase");
}

TEST_F(TilesetNameResolverTest, FlatCaseShorthand)
{
    auto result = resolve_tileset_name("secretbase", tilesets_, &formatter_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "gTileset_SecretBase");
}

TEST_F(TilesetNameResolverTest, FuzzyFullName)
{
    auto result = resolve_tileset_name("gTileset_secret_base", tilesets_, &formatter_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "gTileset_SecretBase");

    // A case-mangled prefix still resolves through the full-name key.
    auto mangled_prefix = resolve_tileset_name("gtileset_secret_base", tilesets_, &formatter_);
    ASSERT_TRUE(mangled_prefix.has_value());
    EXPECT_EQ(mangled_prefix.value(), "gTileset_SecretBase");
}

TEST_F(TilesetNameResolverTest, MultiWordShorthand)
{
    auto result = resolve_tileset_name("petalburg_city", tilesets_, &formatter_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "gTileset_PetalburgCity");
}

TEST_F(TilesetNameResolverTest, NoMatch)
{
    auto result = resolve_tileset_name("fortree", tilesets_, &formatter_);
    ASSERT_FALSE(result.has_value());
    const auto message = result.error().join(formatter_);
    EXPECT_NE(message.find("'fortree'"), std::string::npos);
    EXPECT_NE(message.find("does not match any tileset"), std::string::npos);
}

TEST_F(TilesetNameResolverTest, EmptyInput)
{
    auto empty = resolve_tileset_name("", tilesets_, &formatter_);
    EXPECT_FALSE(empty.has_value());

    auto bare_prefix = resolve_tileset_name("gTileset_", tilesets_, &formatter_);
    EXPECT_FALSE(bare_prefix.has_value());
}

TEST_F(TilesetNameResolverTest, EmptyTilesetSet)
{
    auto result = resolve_tileset_name("SecretBase", {}, &formatter_);
    EXPECT_FALSE(result.has_value());
}

TEST_F(TilesetNameResolverTest, AmbiguousMatch)
{
    const std::set<std::string> tilesets{"gTileset_SecretBase", "gTileset_Secret_Base"};

    auto result = resolve_tileset_name("secret_base", tilesets, &formatter_);
    ASSERT_FALSE(result.has_value());
    const auto message = result.error().join(formatter_);
    EXPECT_NE(message.find("ambiguous"), std::string::npos);
    EXPECT_NE(message.find("'gTileset_SecretBase'"), std::string::npos);
    EXPECT_NE(message.find("'gTileset_Secret_Base'"), std::string::npos);
}

TEST_F(TilesetNameResolverTest, ExactNameBeatsFuzzySiblings)
{
    const std::set<std::string> tilesets{"gTileset_SecretBase", "gTileset_Secret_Base"};

    auto result = resolve_tileset_name("gTileset_Secret_Base", tilesets, &formatter_);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "gTileset_Secret_Base");
}
