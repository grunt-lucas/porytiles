#include "gtest/gtest.h"

#include <fstream>
#include <sstream>

#include "porytiles2/utilities/stream_digest.hpp"

using namespace porytiles2;

class StreamDigestTest : public ::testing::Test {
  protected:
    StreamDigest digest_;
};

TEST_F(StreamDigestTest, EmptyStream)
{
    std::istringstream empty_stream("");
    std::string hash = digest_.digest(empty_stream);
    EXPECT_EQ(hash, "d41d8cd98f00b204e9800998ecf8427e");
}

TEST_F(StreamDigestTest, SimpleString)
{
    std::istringstream stream("The quick brown fox jumps over the lazy dog");
    std::string hash = digest_.digest(stream);
    EXPECT_EQ(hash, "9e107d9d372bb6826bd81d3542a419d6");
}

TEST_F(StreamDigestTest, SingleCharacter)
{
    std::istringstream stream("a");
    std::string hash = digest_.digest(stream);
    EXPECT_EQ(hash, "0cc175b9c0f1b6a831c399e269772661");
}

TEST_F(StreamDigestTest, Alphabet)
{
    std::istringstream stream("abcdefghijklmnopqrstuvwxyz");
    std::string hash = digest_.digest(stream);
    EXPECT_EQ(hash, "c3fcd3d76192e4007dfb496cca67e13b");
}

TEST_F(StreamDigestTest, AlphanumericLong)
{
    std::istringstream stream("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    std::string hash = digest_.digest(stream);
    EXPECT_EQ(hash, "d174ab98d277d9f5a5611c2c9f419d9f");
}

TEST_F(StreamDigestTest, NumericSequence)
{
    std::istringstream stream("12345678901234567890123456789012345678901234567890123456789012345678901234567890");
    std::string hash = digest_.digest(stream);
    EXPECT_EQ(hash, "57edf4a22be3c955ac49da2e2107b67a");
}

TEST_F(StreamDigestTest, TestFile01Pal)
{
    std::ifstream file("./Resources/Tests/unit/utilities/01.pal", std::ios::binary);
    ASSERT_TRUE(file.is_open());
    std::string hash = digest_.digest(file);
    EXPECT_EQ(hash, "86bc455280dcf9edf8de64b6b5a93034");
}

TEST_F(StreamDigestTest, TestFileMetatileAttributes)
{
    std::ifstream file("./Resources/Tests/unit/utilities/metatile_attributes.bin", std::ios::binary);
    ASSERT_TRUE(file.is_open());
    std::string hash = digest_.digest(file);
    EXPECT_EQ(hash, "2cf616d7123c5677c3bd3f97c51b8833");
}

TEST_F(StreamDigestTest, TestFileMetatiles)
{
    std::ifstream file("./Resources/Tests/unit/utilities/metatiles.bin", std::ios::binary);
    ASSERT_TRUE(file.is_open());
    std::string hash = digest_.digest(file);
    EXPECT_EQ(hash, "0e41bc7e32ffe8949bf2235819c54df5");
}

TEST_F(StreamDigestTest, TestFileTilesPng)
{
    std::ifstream file("./Resources/Tests/unit/utilities/tiles.png", std::ios::binary);
    ASSERT_TRUE(file.is_open());
    std::string hash = digest_.digest(file);
    EXPECT_EQ(hash, "76ed48af8ac84f00ab6ae40c8c75e367");
}