#include "gtest/gtest.h"

#include <string>

#include "porytiles/domain/models/animation.hpp"
#include "porytiles/utilities/dynamic_cased_name.hpp"
#include "porytiles/utilities/string_utils.hpp"

using namespace porytiles;

class StringUtilsTest : public ::testing::Test {};

TEST_F(StringUtilsTest, ToPascalCase)
{
    // Empty string
    EXPECT_EQ(to_pascal_case(""), "");

    // Single word
    EXPECT_EQ(to_pascal_case("hello"), "Hello");
    EXPECT_EQ(to_pascal_case("Hello"), "Hello");

    // Snake case input
    EXPECT_EQ(to_pascal_case("hello_world"), "HelloWorld");
    EXPECT_EQ(to_pascal_case("foo_bar_baz"), "FooBarBaz");

    // Kebab case input
    EXPECT_EQ(to_pascal_case("hello-world"), "HelloWorld");
    EXPECT_EQ(to_pascal_case("foo-bar-baz"), "FooBarBaz");

    // Space separated input
    EXPECT_EQ(to_pascal_case("hello world"), "HelloWorld");
    EXPECT_EQ(to_pascal_case("foo bar baz"), "FooBarBaz");

    // Mixed separators
    EXPECT_EQ(to_pascal_case("hello_world-test case"), "HelloWorldTestCase");

    // Consecutive separators
    EXPECT_EQ(to_pascal_case("hello__world"), "HelloWorld");
    EXPECT_EQ(to_pascal_case("hello--world"), "HelloWorld");

    // Leading/trailing separators
    EXPECT_EQ(to_pascal_case("_hello_"), "Hello");
    EXPECT_EQ(to_pascal_case("-hello-"), "Hello");

    // Preserves existing case (capitalizes after separators but preserves other chars)
    EXPECT_EQ(to_pascal_case("helloWORLD"), "HelloWORLD");
    EXPECT_EQ(to_pascal_case("HELLO"), "HELLO");
    EXPECT_EQ(to_pascal_case("HELLO_WORLD"), "HELLOWORLD");
    EXPECT_EQ(to_pascal_case("HELLO_woRLD"), "HELLOWoRLD");

    // Funky vanilla gTileset_General animation name
    EXPECT_EQ(to_pascal_case("tv_turned_on"), "TvTurnedOn");
}

TEST_F(StringUtilsTest, ToSnakeCase)
{
    // Empty string
    EXPECT_EQ(to_snake_case(""), "");

    // Single word
    EXPECT_EQ(to_snake_case("hello"), "hello");
    EXPECT_EQ(to_snake_case("Hello"), "hello");

    // Pascal case input
    EXPECT_EQ(to_snake_case("HelloWorld"), "hello_world");
    EXPECT_EQ(to_snake_case("FooBarBaz"), "foo_bar_baz");

    // Camel case input
    EXPECT_EQ(to_snake_case("helloWorld"), "hello_world");
    EXPECT_EQ(to_snake_case("fooBarBaz"), "foo_bar_baz");

    // Already snake case
    EXPECT_EQ(to_snake_case("hello_world"), "hello_world");
    EXPECT_EQ(to_snake_case("foo_bar_baz"), "foo_bar_baz");

    // Consecutive uppercase (acronyms)
    EXPECT_EQ(to_snake_case("XMLParser"), "xml_parser");
    EXPECT_EQ(to_snake_case("parseXMLDocument"), "parse_xml_document");
    EXPECT_EQ(to_snake_case("HTTPSConnection"), "https_connection");

    // All uppercase
    EXPECT_EQ(to_snake_case("XML"), "xml");
    EXPECT_EQ(to_snake_case("HTTP"), "http");

    // Kebab case input
    EXPECT_EQ(to_snake_case("hello-world"), "hello_world");
    EXPECT_EQ(to_snake_case("foo-bar-baz"), "foo_bar_baz");

    // Space separated input
    EXPECT_EQ(to_snake_case("hello world"), "hello_world");
    EXPECT_EQ(to_snake_case("foo bar baz"), "foo_bar_baz");

    // Mixed separators
    EXPECT_EQ(to_snake_case("hello_world-test case"), "hello_world_test_case");

    // Consecutive separators
    EXPECT_EQ(to_snake_case("hello__world"), "hello_world");
    EXPECT_EQ(to_snake_case("hello--world"), "hello_world");

    // Leading/trailing separators
    EXPECT_EQ(to_snake_case("_hello_"), "hello");
    EXPECT_EQ(to_snake_case("-hello-"), "hello");

    // Mixed case with separators
    EXPECT_EQ(to_snake_case("Hello_World"), "hello_world");
    EXPECT_EQ(to_snake_case("helloWorld_Test"), "hello_world_test");

    // Funky vanilla gTileset_General animation name
    EXPECT_EQ(to_snake_case("TVTurnedOn"), "tv_turned_on");
    EXPECT_EQ(to_snake_case("TvTurnedOn"), "tv_turned_on");
}

TEST_F(StringUtilsTest, TrimPrefix)
{
    // Empty string and empty prefix
    EXPECT_EQ(trim_prefix("", ""), "");
    EXPECT_EQ(trim_prefix("hello", ""), "hello");
    EXPECT_EQ(trim_prefix("", "hello"), "");

    // Prefix present
    EXPECT_EQ(trim_prefix("hello_world", "hello_"), "world");
    EXPECT_EQ(trim_prefix("prefix_suffix", "prefix_"), "suffix");

    // Prefix not present
    EXPECT_EQ(trim_prefix("hello_world", "foo"), "hello_world");
    EXPECT_EQ(trim_prefix("hello_world", "world"), "hello_world");

    // Exact match
    EXPECT_EQ(trim_prefix("hello", "hello"), "");

    // Prefix longer than string
    EXPECT_EQ(trim_prefix("hello", "hello_world"), "hello");

    // Partial match
    EXPECT_EQ(trim_prefix("hello", "hel"), "lo");
    EXPECT_EQ(trim_prefix("hello", "help"), "hello");

    // Case sensitive
    EXPECT_EQ(trim_prefix("Hello_World", "hello_"), "Hello_World");
    EXPECT_EQ(trim_prefix("Hello_World", "Hello_"), "World");
}

TEST_F(StringUtilsTest, JoinQuoted)
{
    // No values yields an empty string rather than stray quotes or a delimiter, so a caller can drop the whole clause
    // by testing the result for emptiness.
    EXPECT_EQ(join_quoted({}), "");

    // A single value gets its quotes but no delimiter.
    EXPECT_EQ(join_quoted({"only"}), "'only'");

    EXPECT_EQ(join_quoted({"a.c", "b.h"}), "'a.c', 'b.h'");
    EXPECT_EQ(join_quoted({"x", "y", "z"}), "'x', 'y', 'z'");

    // The delimiter replaces the default without affecting the quoting.
    EXPECT_EQ(join_quoted({"x", "y"}, " and "), "'x' and 'y'");
    EXPECT_EQ(join_quoted({"x", "y"}, ""), "'x''y'");

    // Values are quoted verbatim: an empty value still yields a quoted empty pair, so the count of quoted items always
    // matches the count of values.
    EXPECT_EQ(join_quoted({""}), "''");
    EXPECT_EQ(join_quoted({"", "b"}), "'', 'b'");
    EXPECT_EQ(join_quoted({"has space", "has'quote"}), "'has space', 'has'quote'");
}

TEST_F(StringUtilsTest, ExtractTilesetCasedNameWithPrefix)
{
    auto result = extract_tileset_cased_name("gTileset_General");
    EXPECT_EQ(result.to_pascal_case(), "General");
    EXPECT_EQ(result.to_snake_case(), "general");
}

TEST_F(StringUtilsTest, ExtractTilesetCasedNameWithoutPrefix)
{
    auto result = extract_tileset_cased_name("General");
    EXPECT_EQ(result.to_pascal_case(), "General");
    EXPECT_EQ(result.to_snake_case(), "general");
}

TEST_F(StringUtilsTest, ExtractTilesetCasedNameCompound)
{
    auto result = extract_tileset_cased_name("gTileset_PetalburgCity");
    EXPECT_EQ(result.to_pascal_case(), "PetalburgCity");
    EXPECT_EQ(result.to_snake_case(), "petalburg_city");
}

TEST_F(StringUtilsTest, ManagedCallbackName)
{
    // snake_case-segmented names (the bug repro): the callback symbol must be PascalCase so it matches the
    // generated function definition and forward declaration, not the raw snake_case shorthand.
    EXPECT_EQ(anim::managed_callback_name("gTileset_velvet_forest"), "InitTilesetAnim_PorytilesManaged_VelvetForest");
    EXPECT_EQ(
        anim::managed_callback_name("gTileset_battle_frontier"), "InitTilesetAnim_PorytilesManaged_BattleFrontier");

    // PascalCase input (every vanilla tileset) still converges to the same result.
    EXPECT_EQ(anim::managed_callback_name("gTileset_General"), "InitTilesetAnim_PorytilesManaged_General");
}
