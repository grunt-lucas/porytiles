#include "gtest/gtest.h"

#include <optional>
#include <sstream>
#include <string>

#include "porytiles2/domain/models/base_game.hpp"

using namespace porytiles2;

TEST(BaseGameTest, ToStringPokeemerald)
{
    EXPECT_EQ(to_string(BaseGame::pokeemerald), "pokeemerald");
}

TEST(BaseGameTest, ToStringPokefirered)
{
    EXPECT_EQ(to_string(BaseGame::pokefirered), "pokefirered");
}

TEST(BaseGameTest, ToStringPokeruby)
{
    EXPECT_EQ(to_string(BaseGame::pokeruby), "pokeruby");
}

TEST(BaseGameTest, ToStringPokeemeraldExpansion)
{
    EXPECT_EQ(to_string(BaseGame::pokeemerald_expansion), "pokeemerald-expansion");
}

TEST(BaseGameTest, FromStrPokeemerald)
{
    auto result = base_game_from_str("pokeemerald");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), BaseGame::pokeemerald);
}

TEST(BaseGameTest, FromStrPokefirered)
{
    auto result = base_game_from_str("pokefirered");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), BaseGame::pokefirered);
}

TEST(BaseGameTest, FromStrPokeruby)
{
    auto result = base_game_from_str("pokeruby");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), BaseGame::pokeruby);
}

TEST(BaseGameTest, FromStrPokeemeraldExpansion)
{
    auto result = base_game_from_str("pokeemerald-expansion");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), BaseGame::pokeemerald_expansion);
}

TEST(BaseGameTest, FromStrInvalid)
{
    EXPECT_EQ(base_game_from_str("invalid"), std::nullopt);
    EXPECT_EQ(base_game_from_str(""), std::nullopt);
    EXPECT_EQ(base_game_from_str("Pokeemerald"), std::nullopt);
    EXPECT_EQ(base_game_from_str("pokeemerald_expansion"), std::nullopt);
}

TEST(BaseGameTest, RoundTrip)
{
    auto result = base_game_from_str(to_string(BaseGame::pokeemerald));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), BaseGame::pokeemerald);

    result = base_game_from_str(to_string(BaseGame::pokefirered));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), BaseGame::pokefirered);

    result = base_game_from_str(to_string(BaseGame::pokeruby));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), BaseGame::pokeruby);

    result = base_game_from_str(to_string(BaseGame::pokeemerald_expansion));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), BaseGame::pokeemerald_expansion);
}

TEST(BaseGameTest, StreamOperator)
{
    std::ostringstream oss;
    oss << BaseGame::pokeemerald;
    EXPECT_EQ(oss.str(), "pokeemerald");

    oss.str("");
    oss << BaseGame::pokefirered;
    EXPECT_EQ(oss.str(), "pokefirered");

    oss.str("");
    oss << BaseGame::pokeruby;
    EXPECT_EQ(oss.str(), "pokeruby");

    oss.str("");
    oss << BaseGame::pokeemerald_expansion;
    EXPECT_EQ(oss.str(), "pokeemerald-expansion");
}

TEST(BaseGameTest, StdFormat)
{
    EXPECT_EQ(std::format("{}", BaseGame::pokeemerald), "pokeemerald");
    EXPECT_EQ(std::format("{}", BaseGame::pokefirered), "pokefirered");
    EXPECT_EQ(std::format("{}", BaseGame::pokeruby), "pokeruby");
    EXPECT_EQ(std::format("{}", BaseGame::pokeemerald_expansion), "pokeemerald-expansion");
}
