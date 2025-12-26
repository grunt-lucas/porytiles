#include "gtest/gtest.h"

#include <string>

#include "porytiles2/utilities/string_utils.hpp"

using namespace porytiles2;

class StringUtilsTest : public ::testing::Test {};

// =====================================================
// to_pascal_case tests
// =====================================================

TEST_F(StringUtilsTest, ToPascalCase_EmptyString)
{
    EXPECT_EQ(to_pascal_case(""), "");
}

TEST_F(StringUtilsTest, ToPascalCase_SingleWord)
{
    EXPECT_EQ(to_pascal_case("hello"), "Hello");
}

TEST_F(StringUtilsTest, ToPascalCase_SingleWordAlreadyCapitalized)
{
    EXPECT_EQ(to_pascal_case("Hello"), "Hello");
}

TEST_F(StringUtilsTest, ToPascalCase_SnakeCaseInput)
{
    EXPECT_EQ(to_pascal_case("hello_world"), "HelloWorld");
    EXPECT_EQ(to_pascal_case("foo_bar_baz"), "FooBarBaz");
}

TEST_F(StringUtilsTest, ToPascalCase_KebabCaseInput)
{
    EXPECT_EQ(to_pascal_case("hello-world"), "HelloWorld");
    EXPECT_EQ(to_pascal_case("foo-bar-baz"), "FooBarBaz");
}

TEST_F(StringUtilsTest, ToPascalCase_SpaceSeparatedInput)
{
    EXPECT_EQ(to_pascal_case("hello world"), "HelloWorld");
    EXPECT_EQ(to_pascal_case("foo bar baz"), "FooBarBaz");
}

TEST_F(StringUtilsTest, ToPascalCase_MixedSeparators)
{
    EXPECT_EQ(to_pascal_case("hello_world-test case"), "HelloWorldTestCase");
}

TEST_F(StringUtilsTest, ToPascalCase_ConsecutiveSeparators)
{
    EXPECT_EQ(to_pascal_case("hello__world"), "HelloWorld");
    EXPECT_EQ(to_pascal_case("hello--world"), "HelloWorld");
}

TEST_F(StringUtilsTest, ToPascalCase_LeadingTrailingSeparators)
{
    EXPECT_EQ(to_pascal_case("_hello_"), "Hello");
    EXPECT_EQ(to_pascal_case("-hello-"), "Hello");
}

TEST_F(StringUtilsTest, ToPascalCase_PreservesExistingCase)
{
    // Note: to_pascal_case capitalizes after separators but preserves other chars
    EXPECT_EQ(to_pascal_case("helloWORLD"), "HelloWORLD");
}

// =====================================================
// to_snake_case tests
// =====================================================

TEST_F(StringUtilsTest, ToSnakeCase_EmptyString)
{
    EXPECT_EQ(to_snake_case(""), "");
}

TEST_F(StringUtilsTest, ToSnakeCase_SingleWord)
{
    EXPECT_EQ(to_snake_case("hello"), "hello");
    EXPECT_EQ(to_snake_case("Hello"), "hello");
}

TEST_F(StringUtilsTest, ToSnakeCase_PascalCaseInput)
{
    EXPECT_EQ(to_snake_case("HelloWorld"), "hello_world");
    EXPECT_EQ(to_snake_case("FooBarBaz"), "foo_bar_baz");
}

TEST_F(StringUtilsTest, ToSnakeCase_CamelCaseInput)
{
    EXPECT_EQ(to_snake_case("helloWorld"), "hello_world");
    EXPECT_EQ(to_snake_case("fooBarBaz"), "foo_bar_baz");
}

TEST_F(StringUtilsTest, ToSnakeCase_AlreadySnakeCase)
{
    EXPECT_EQ(to_snake_case("hello_world"), "hello_world");
    EXPECT_EQ(to_snake_case("foo_bar_baz"), "foo_bar_baz");
}

TEST_F(StringUtilsTest, ToSnakeCase_ConsecutiveUppercase)
{
    EXPECT_EQ(to_snake_case("XMLParser"), "xml_parser");
    EXPECT_EQ(to_snake_case("parseXMLDocument"), "parse_xml_document");
    EXPECT_EQ(to_snake_case("HTTPSConnection"), "https_connection");
}

TEST_F(StringUtilsTest, ToSnakeCase_AllUppercase)
{
    EXPECT_EQ(to_snake_case("XML"), "xml");
    EXPECT_EQ(to_snake_case("HTTP"), "http");
}

TEST_F(StringUtilsTest, ToSnakeCase_KebabCaseInput)
{
    EXPECT_EQ(to_snake_case("hello-world"), "hello_world");
    EXPECT_EQ(to_snake_case("foo-bar-baz"), "foo_bar_baz");
}

TEST_F(StringUtilsTest, ToSnakeCase_SpaceSeparatedInput)
{
    EXPECT_EQ(to_snake_case("hello world"), "hello_world");
    EXPECT_EQ(to_snake_case("foo bar baz"), "foo_bar_baz");
}

TEST_F(StringUtilsTest, ToSnakeCase_MixedSeparators)
{
    EXPECT_EQ(to_snake_case("hello_world-test case"), "hello_world_test_case");
}

TEST_F(StringUtilsTest, ToSnakeCase_ConsecutiveSeparators)
{
    EXPECT_EQ(to_snake_case("hello__world"), "hello_world");
    EXPECT_EQ(to_snake_case("hello--world"), "hello_world");
}

TEST_F(StringUtilsTest, ToSnakeCase_LeadingTrailingSeparators)
{
    EXPECT_EQ(to_snake_case("_hello_"), "hello");
    EXPECT_EQ(to_snake_case("-hello-"), "hello");
}

TEST_F(StringUtilsTest, ToSnakeCase_MixedCaseWithSeparators)
{
    EXPECT_EQ(to_snake_case("Hello_World"), "hello_world");
    EXPECT_EQ(to_snake_case("helloWorld_Test"), "hello_world_test");
}

// =====================================================
// trim_prefix tests
// =====================================================

TEST_F(StringUtilsTest, TrimPrefix_EmptyStringAndEmptyPrefix)
{
    EXPECT_EQ(trim_prefix("", ""), "");
}

TEST_F(StringUtilsTest, TrimPrefix_EmptyPrefix)
{
    EXPECT_EQ(trim_prefix("hello", ""), "hello");
}

TEST_F(StringUtilsTest, TrimPrefix_EmptyString)
{
    EXPECT_EQ(trim_prefix("", "hello"), "");
}

TEST_F(StringUtilsTest, TrimPrefix_PrefixPresent)
{
    EXPECT_EQ(trim_prefix("hello_world", "hello_"), "world");
    EXPECT_EQ(trim_prefix("prefix_suffix", "prefix_"), "suffix");
}

TEST_F(StringUtilsTest, TrimPrefix_PrefixNotPresent)
{
    EXPECT_EQ(trim_prefix("hello_world", "foo"), "hello_world");
    EXPECT_EQ(trim_prefix("hello_world", "world"), "hello_world");
}

TEST_F(StringUtilsTest, TrimPrefix_ExactMatch)
{
    EXPECT_EQ(trim_prefix("hello", "hello"), "");
}

TEST_F(StringUtilsTest, TrimPrefix_PrefixLongerThanString)
{
    EXPECT_EQ(trim_prefix("hello", "hello_world"), "hello");
}

TEST_F(StringUtilsTest, TrimPrefix_PartialMatch)
{
    // "hel" is a prefix, but "help" is not
    EXPECT_EQ(trim_prefix("hello", "hel"), "lo");
    EXPECT_EQ(trim_prefix("hello", "help"), "hello");
}

TEST_F(StringUtilsTest, TrimPrefix_CaseSensitive)
{
    EXPECT_EQ(trim_prefix("Hello_World", "hello_"), "Hello_World");
    EXPECT_EQ(trim_prefix("Hello_World", "Hello_"), "World");
}
