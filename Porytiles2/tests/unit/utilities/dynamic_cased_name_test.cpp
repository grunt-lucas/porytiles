#include "porytiles2/utilities/dynamic_cased_name.hpp"

#include <format>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "gtest/gtest.h"

using namespace porytiles2;

class DynamicCasedNameTest : public ::testing::Test {};

TEST_F(DynamicCasedNameTest, FromSnakeCase)
{
    auto name = DynamicCasedName::from_snake_case("my_tileset");
    ASSERT_EQ(name.segments().size(), 2);
    EXPECT_EQ(name.segments()[0], std::vector<std::string>{"my"});
    EXPECT_EQ(name.segments()[1], std::vector<std::string>{"tileset"});
}

TEST_F(DynamicCasedNameTest, FromPascalCase)
{
    auto name = DynamicCasedName::from_pascal_case("AnotherTileset");
    ASSERT_EQ(name.segments().size(), 1);
    EXPECT_EQ(name.segments()[0], (std::vector<std::string>{"another", "tileset"}));
}

TEST_F(DynamicCasedNameTest, FromCIdentifier)
{
    auto name = DynamicCasedName::from_c_identifier("Water_Current_LandWatersEdge");
    ASSERT_EQ(name.segments().size(), 3);
    EXPECT_EQ(name.segments()[0], std::vector<std::string>{"water"});
    EXPECT_EQ(name.segments()[1], std::vector<std::string>{"current"});
    EXPECT_EQ(name.segments()[2], (std::vector<std::string>{"land", "waters", "edge"}));
}

TEST_F(DynamicCasedNameTest, FromFlatCase)
{
    auto name = DynamicCasedName::from_flat_case("sandwatersedge");
    ASSERT_EQ(name.segments().size(), 1);
    EXPECT_EQ(name.segments()[0], std::vector<std::string>{"sandwatersedge"});
}

TEST_F(DynamicCasedNameTest, FromPascalCaseWithAcronyms)
{
    auto name = DynamicCasedName::from_pascal_case("TVTurnedOn");
    ASSERT_EQ(name.segments().size(), 1);
    EXPECT_EQ(name.segments()[0], (std::vector<std::string>{"tv", "turned", "on"}));
}

TEST_F(DynamicCasedNameTest, FromPascalCaseXMLParser)
{
    auto name = DynamicCasedName::from_pascal_case("XMLParser");
    ASSERT_EQ(name.segments().size(), 1);
    EXPECT_EQ(name.segments()[0], (std::vector<std::string>{"xml", "parser"}));
}

TEST_F(DynamicCasedNameTest, FromSnakeCaseSingleWord)
{
    auto name = DynamicCasedName::from_snake_case("general");
    ASSERT_EQ(name.segments().size(), 1);
    EXPECT_EQ(name.segments()[0], std::vector<std::string>{"general"});
}

TEST_F(DynamicCasedNameTest, AutoDetectSnakeCase)
{
    DynamicCasedName name{"my_tileset"};
    ASSERT_EQ(name.segments().size(), 2);
    EXPECT_EQ(name.segments()[0], std::vector<std::string>{"my"});
    EXPECT_EQ(name.segments()[1], std::vector<std::string>{"tileset"});
}

TEST_F(DynamicCasedNameTest, AutoDetectPascalCase)
{
    DynamicCasedName name{"AnotherTileset"};
    ASSERT_EQ(name.segments().size(), 1);
    EXPECT_EQ(name.segments()[0], (std::vector<std::string>{"another", "tileset"}));
}

TEST_F(DynamicCasedNameTest, AutoDetectCIdentifier)
{
    DynamicCasedName name{"Water_Current_LandWatersEdge"};
    ASSERT_EQ(name.segments().size(), 3);
    EXPECT_EQ(name.segments()[0], std::vector<std::string>{"water"});
    EXPECT_EQ(name.segments()[1], std::vector<std::string>{"current"});
    EXPECT_EQ(name.segments()[2], (std::vector<std::string>{"land", "waters", "edge"}));
}

TEST_F(DynamicCasedNameTest, AutoDetectFlatCase)
{
    DynamicCasedName name{"sandwatersedge"};
    ASSERT_EQ(name.segments().size(), 1);
    EXPECT_EQ(name.segments()[0], std::vector<std::string>{"sandwatersedge"});
}

TEST_F(DynamicCasedNameTest, OutputFromSnakeCase)
{
    auto name = DynamicCasedName::from_snake_case("my_tileset");
    EXPECT_EQ(name.to_snake_case(), "my_tileset");
    EXPECT_EQ(name.to_pascal_case(), "MyTileset");
    EXPECT_EQ(name.to_c_identifier(), "My_Tileset");
    EXPECT_EQ(name.to_flat_case(), "mytileset");
}

TEST_F(DynamicCasedNameTest, OutputFromPascalCase)
{
    auto name = DynamicCasedName::from_pascal_case("AnotherTileset");
    EXPECT_EQ(name.to_snake_case(), "another_tileset");
    EXPECT_EQ(name.to_pascal_case(), "AnotherTileset");
    EXPECT_EQ(name.to_c_identifier(), "AnotherTileset");
    EXPECT_EQ(name.to_flat_case(), "anothertileset");
}

TEST_F(DynamicCasedNameTest, OutputFromCIdentifier)
{
    auto name = DynamicCasedName::from_c_identifier("Water_Current_LandWatersEdge");
    EXPECT_EQ(name.to_snake_case(), "water_current_land_waters_edge");
    EXPECT_EQ(name.to_pascal_case(), "WaterCurrentLandWatersEdge");
    EXPECT_EQ(name.to_c_identifier(), "Water_Current_LandWatersEdge");
    EXPECT_EQ(name.to_flat_case(), "watercurrentlandwatersedge");
}

TEST_F(DynamicCasedNameTest, OutputFromFlatCase)
{
    auto name = DynamicCasedName::from_flat_case("sandwatersedge");
    EXPECT_EQ(name.to_snake_case(), "sandwatersedge");
    EXPECT_EQ(name.to_pascal_case(), "Sandwatersedge");
    EXPECT_EQ(name.to_c_identifier(), "Sandwatersedge");
    EXPECT_EQ(name.to_flat_case(), "sandwatersedge");
}

TEST_F(DynamicCasedNameTest, RoundTripSnakeCase)
{
    std::string input = "water_current_land_waters_edge";
    EXPECT_EQ(DynamicCasedName::from_snake_case(input).to_snake_case(), input);
}

TEST_F(DynamicCasedNameTest, RoundTripPascalCase)
{
    std::string input = "WaterCurrentLandWatersEdge";
    EXPECT_EQ(DynamicCasedName::from_pascal_case(input).to_pascal_case(), input);
}

TEST_F(DynamicCasedNameTest, RoundTripCIdentifier)
{
    std::string input = "Water_Current_LandWatersEdge";
    EXPECT_EQ(DynamicCasedName::from_c_identifier(input).to_c_identifier(), input);
}

TEST_F(DynamicCasedNameTest, RoundTripFlatCase)
{
    std::string input = "sandwatersedge";
    EXPECT_EQ(DynamicCasedName::from_flat_case(input).to_flat_case(), input);
}

TEST_F(DynamicCasedNameTest, RoundTripPascalCaseWithAcronym)
{
    std::string input = "TVTurnedOn";
    EXPECT_EQ(DynamicCasedName::from_pascal_case(input).to_pascal_case(), "TvTurnedOn");
}

TEST_F(DynamicCasedNameTest, RoundTripPascalCaseXML)
{
    std::string input = "XMLParser";
    EXPECT_EQ(DynamicCasedName::from_pascal_case(input).to_pascal_case(), "XmlParser");
}

TEST_F(DynamicCasedNameTest, EqualityAcrossFormats)
{
    auto from_snake = DynamicCasedName::from_snake_case("water_current");
    auto from_pascal = DynamicCasedName::from_pascal_case("WaterCurrent");
    EXPECT_EQ(from_snake, from_pascal);
    EXPECT_EQ(from_snake.canonical(), from_pascal.canonical());
}

TEST_F(DynamicCasedNameTest, EqualitySnakeAndFlat)
{
    auto from_snake = DynamicCasedName::from_snake_case("my_tileset");
    auto from_flat = DynamicCasedName::from_flat_case("mytileset");
    EXPECT_EQ(from_snake, from_flat);
}

TEST_F(DynamicCasedNameTest, InequalityDifferentNames)
{
    auto name1 = DynamicCasedName::from_snake_case("water_current");
    auto name2 = DynamicCasedName::from_snake_case("land_waters_edge");
    EXPECT_NE(name1, name2);
}

TEST_F(DynamicCasedNameTest, OrderingWithSpaceship)
{
    auto a = DynamicCasedName::from_snake_case("alpha");
    auto b = DynamicCasedName::from_snake_case("beta");
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a <= a);
    EXPECT_TRUE(b >= a);
}

TEST_F(DynamicCasedNameTest, UsableInStdMap)
{
    std::map<DynamicCasedName, int> map;
    map[DynamicCasedName::from_snake_case("alpha")] = 1;
    map[DynamicCasedName::from_pascal_case("Beta")] = 2;
    map[DynamicCasedName::from_c_identifier("Gamma_Delta")] = 3;

    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map[DynamicCasedName::from_flat_case("alpha")], 1);
    EXPECT_EQ(map[DynamicCasedName::from_flat_case("beta")], 2);
    EXPECT_EQ(map[DynamicCasedName::from_flat_case("gammadelta")], 3);
}

TEST_F(DynamicCasedNameTest, UsableInStdUnorderedMap)
{
    std::unordered_map<DynamicCasedName, int> map;
    map[DynamicCasedName::from_snake_case("alpha")] = 1;
    map[DynamicCasedName::from_pascal_case("Beta")] = 2;

    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map[DynamicCasedName::from_flat_case("alpha")], 1);
    EXPECT_EQ(map[DynamicCasedName::from_flat_case("beta")], 2);
}

TEST_F(DynamicCasedNameTest, HashConsistency)
{
    auto from_snake = DynamicCasedName::from_snake_case("water_current");
    auto from_pascal = DynamicCasedName::from_pascal_case("WaterCurrent");

    std::hash<DynamicCasedName> hasher;
    EXPECT_EQ(hasher(from_snake), hasher(from_pascal));
}

TEST_F(DynamicCasedNameTest, EmptyString)
{
    DynamicCasedName name{""};
    EXPECT_TRUE(name.empty());
    EXPECT_EQ(name.to_snake_case(), "");
    EXPECT_EQ(name.to_pascal_case(), "");
    EXPECT_EQ(name.to_c_identifier(), "");
    EXPECT_EQ(name.to_flat_case(), "");
    EXPECT_EQ(name.canonical(), "");
}

TEST_F(DynamicCasedNameTest, EmptyStringFactories)
{
    EXPECT_TRUE(DynamicCasedName::from_snake_case("").empty());
    EXPECT_TRUE(DynamicCasedName::from_pascal_case("").empty());
    EXPECT_TRUE(DynamicCasedName::from_c_identifier("").empty());
    EXPECT_TRUE(DynamicCasedName::from_flat_case("").empty());
}

TEST_F(DynamicCasedNameTest, DefaultConstructed)
{
    DynamicCasedName name;
    EXPECT_TRUE(name.empty());
    EXPECT_EQ(name.canonical(), "");
}

TEST_F(DynamicCasedNameTest, SingleCharLowercase)
{
    DynamicCasedName name{"a"};
    EXPECT_FALSE(name.empty());
    EXPECT_EQ(name.to_snake_case(), "a");
    EXPECT_EQ(name.to_pascal_case(), "A");
    EXPECT_EQ(name.to_flat_case(), "a");
}

TEST_F(DynamicCasedNameTest, SingleCharUppercase)
{
    DynamicCasedName name{"A"};
    EXPECT_FALSE(name.empty());
    EXPECT_EQ(name.to_snake_case(), "a");
    EXPECT_EQ(name.to_pascal_case(), "A");
    EXPECT_EQ(name.to_flat_case(), "a");
}

TEST_F(DynamicCasedNameTest, LeadingTrailingUnderscores)
{
    auto name = DynamicCasedName::from_snake_case("_hello_world_");
    EXPECT_EQ(name.to_snake_case(), "hello_world");
    EXPECT_EQ(name.to_pascal_case(), "HelloWorld");
}

TEST_F(DynamicCasedNameTest, ConsecutiveUnderscores)
{
    auto name = DynamicCasedName::from_snake_case("hello__world");
    EXPECT_EQ(name.to_snake_case(), "hello_world");
}

TEST_F(DynamicCasedNameTest, AllUppercaseAcronymHTTP)
{
    auto name = DynamicCasedName::from_pascal_case("HTTP");
    ASSERT_EQ(name.segments().size(), 1);
    EXPECT_EQ(name.segments()[0], std::vector<std::string>{"http"});
    EXPECT_EQ(name.to_snake_case(), "http");
    EXPECT_EQ(name.to_pascal_case(), "Http");
}

TEST_F(DynamicCasedNameTest, AllUppercaseAcronymXML)
{
    auto name = DynamicCasedName::from_pascal_case("XML");
    EXPECT_EQ(name.to_snake_case(), "xml");
    EXPECT_EQ(name.to_pascal_case(), "Xml");
}

TEST_F(DynamicCasedNameTest, AcronymFollowedByWord)
{
    auto name = DynamicCasedName::from_pascal_case("XMLParser");
    EXPECT_EQ(name.to_snake_case(), "xml_parser");
    EXPECT_EQ(name.to_pascal_case(), "XmlParser");
}

TEST_F(DynamicCasedNameTest, HTTPSConnection)
{
    auto name = DynamicCasedName::from_pascal_case("HTTPSConnection");
    EXPECT_EQ(name.to_snake_case(), "https_connection");
}

TEST_F(DynamicCasedNameTest, RealWorldGeneral)
{
    DynamicCasedName name{"General"};
    EXPECT_EQ(name.to_snake_case(), "general");
    EXPECT_EQ(name.to_pascal_case(), "General");
    EXPECT_EQ(name.to_c_identifier(), "General");
    EXPECT_EQ(name.to_flat_case(), "general");
}

TEST_F(DynamicCasedNameTest, RealWorldFlower)
{
    DynamicCasedName name{"Flower"};
    EXPECT_EQ(name.to_snake_case(), "flower");
    EXPECT_EQ(name.to_pascal_case(), "Flower");
}

TEST_F(DynamicCasedNameTest, RealWorldWaterCurrentLandWatersEdge)
{
    DynamicCasedName name{"Water_Current_LandWatersEdge"};
    EXPECT_EQ(name.to_snake_case(), "water_current_land_waters_edge");
    EXPECT_EQ(name.to_pascal_case(), "WaterCurrentLandWatersEdge");
    EXPECT_EQ(name.to_c_identifier(), "Water_Current_LandWatersEdge");
    EXPECT_EQ(name.to_flat_case(), "watercurrentlandwatersedge");
}

TEST_F(DynamicCasedNameTest, RealWorldSandWatersEdge)
{
    auto name = DynamicCasedName::from_flat_case("sandwatersedge");
    EXPECT_EQ(name.to_snake_case(), "sandwatersedge");
    EXPECT_EQ(name.to_pascal_case(), "Sandwatersedge");
    EXPECT_EQ(name.to_flat_case(), "sandwatersedge");
}

TEST_F(DynamicCasedNameTest, RealWorldMotorizedDoor)
{
    auto name = DynamicCasedName::from_flat_case("motorizeddoor");
    EXPECT_EQ(name.to_snake_case(), "motorizeddoor");
    EXPECT_EQ(name.to_pascal_case(), "Motorizeddoor");
    EXPECT_EQ(name.to_flat_case(), "motorizeddoor");
}

TEST_F(DynamicCasedNameTest, RealWorldTVTurnedOnVsTvTurnedOn)
{
    auto tv_upper = DynamicCasedName::from_pascal_case("TVTurnedOn");
    auto tv_lower = DynamicCasedName::from_pascal_case("TvTurnedOn");
    EXPECT_EQ(tv_upper, tv_lower);
    EXPECT_EQ(tv_upper.canonical(), "tvturnedon");
}

TEST_F(DynamicCasedNameTest, ToStringFunction)
{
    auto name = DynamicCasedName::from_snake_case("my_tileset");
    EXPECT_EQ(to_string(name), "my_tileset");
}

TEST_F(DynamicCasedNameTest, StdFormat)
{
    auto name = DynamicCasedName::from_pascal_case("HelloWorld");
    std::string formatted = std::format("{}", name);
    EXPECT_EQ(formatted, "hello_world");
}

TEST_F(DynamicCasedNameTest, StdFormatWithSurroundingText)
{
    auto name = DynamicCasedName::from_pascal_case("MyTileset");
    std::string formatted = std::format("name={}", name);
    EXPECT_EQ(formatted, "name=my_tileset");
}

TEST_F(DynamicCasedNameTest, StreamOperator)
{
    auto name = DynamicCasedName::from_snake_case("hello_world");
    std::ostringstream ss;
    ss << name;
    EXPECT_EQ(ss.str(), "hello_world");
}
