#include "gtest/gtest.h"

#include <cstdint>
#include <limits>
#include <string>

#include "porytiles/utilities/parse_int.hpp"

using namespace porytiles;

class ParseIntTest : public ::testing::Test {};

TEST_F(ParseIntTest, ParsesDecimalValues)
{
    EXPECT_EQ(parse_int<int>("0").value(), 0);
    EXPECT_EQ(parse_int<int>("42").value(), 42);
    EXPECT_EQ(parse_int<int>("-7").value(), -7);
    EXPECT_EQ(parse_int<std::size_t>("1234").value(), 1234u);
}

TEST_F(ParseIntTest, Base0AutoDetectsHexAndOctal)
{
    EXPECT_EQ(parse_int<int>("0x1F").value(), 31);
    EXPECT_EQ(parse_int<int>("010").value(), 8);
    EXPECT_EQ(parse_int<std::uint32_t>("0xFFFFFFFF").value(), 0xFFFFFFFFu);
}

TEST_F(ParseIntTest, ParsesValuesAboveIntMax)
{
    // A value above INT_MAX but within the requested type's range must parse; the old std::stoi-based implementation
    // threw out_of_range here regardless of T.
    EXPECT_EQ(parse_int<long long>("3000000000").value(), 3000000000LL);
    EXPECT_EQ(parse_int<std::uint32_t>("3000000000").value(), 3000000000u);
    EXPECT_EQ(parse_int<std::size_t>("4294967295").value(), 4294967295u);
    EXPECT_EQ(
        parse_int<long long>(std::to_string(std::numeric_limits<long long>::max())).value(),
        std::numeric_limits<long long>::max());
}

TEST_F(ParseIntTest, RejectsValuesOutsideRequestedType)
{
    EXPECT_FALSE(parse_int<int>("3000000000").has_value());
    EXPECT_FALSE(parse_int<std::uint32_t>("4294967296").has_value());
    EXPECT_FALSE(parse_int<std::uint8_t>("256").has_value());
    EXPECT_EQ(parse_int<std::uint8_t>("255").value(), 255u);
}

TEST_F(ParseIntTest, RejectsNegativeValuesForUnsignedTypes)
{
    // The old implementation parsed -1 with std::stoi and let the implicit conversion wrap it to SIZE_MAX.
    EXPECT_FALSE(parse_int<std::size_t>("-1").has_value());
    EXPECT_FALSE(parse_int<std::uint32_t>("-1").has_value());
    EXPECT_EQ(parse_int<int>("-1").value(), -1);
}

TEST_F(ParseIntTest, RejectsNonIntegralStrings)
{
    EXPECT_FALSE(parse_int<int>("").has_value());
    EXPECT_FALSE(parse_int<int>("abc").has_value());
    EXPECT_FALSE(parse_int<int>("12abc").has_value());
    EXPECT_FALSE(parse_int<int>("1.5").has_value());
    EXPECT_FALSE(parse_int<int>("12 ").has_value());
}

TEST_F(ParseIntTest, HandlesNonNullTerminatedStringView)
{
    // A string_view sliced out of a larger buffer is not null-terminated at its end; the parse must respect the
    // view's bounds instead of reading through to the underlying buffer's terminator.
    const std::string buffer{"123456"};
    const std::string_view view{buffer.data(), 3};
    EXPECT_EQ(parse_int<int>(view).value(), 123);
}
