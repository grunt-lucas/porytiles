#include "gtest/gtest.h"

#include <cstdint>
#include <filesystem>
#include <fstream>

#include "porytiles2/domain/models/layer.hpp"
#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/infra/algorithms/porymap_artifact_parsers.hpp"

using namespace porytiles2;

namespace {

/**
 * @brief Encodes a FireRed metatile attribute into 4 bytes (little-endian).
 */
void write_firered_attribute(
    std::ofstream &out,
    std::uint16_t behavior,
    std::uint8_t terrain,
    std::uint8_t attribute_2,
    std::uint8_t attribute_3,
    std::uint8_t encounter_type,
    std::uint8_t attribute_5,
    unsigned int layer_type,
    bool attribute_7)
{
    const auto value = static_cast<std::uint32_t>(
        (static_cast<std::uint32_t>(behavior) & 0x1FF) | ((static_cast<std::uint32_t>(terrain) & 0x1F) << 9) |
        ((static_cast<std::uint32_t>(attribute_2) & 0x0F) << 14) |
        ((static_cast<std::uint32_t>(attribute_3) & 0x3F) << 18) |
        ((static_cast<std::uint32_t>(encounter_type) & 0x07) << 24) |
        ((static_cast<std::uint32_t>(attribute_5) & 0x03) << 27) |
        ((static_cast<std::uint32_t>(layer_type) & 0x03) << 29) |
        ((static_cast<std::uint32_t>(attribute_7) & 0x01) << 31));
    out << static_cast<std::uint8_t>(value);
    out << static_cast<std::uint8_t>(value >> 8);
    out << static_cast<std::uint8_t>(value >> 16);
    out << static_cast<std::uint8_t>(value >> 24);
}

} // namespace

class FireRedMetatileAttributeParserTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        test_dir_ = std::filesystem::temp_directory_path() / "porytiles_firered_parser_test";
        std::filesystem::create_directories(test_dir_);
        test_file_ = test_dir_ / "metatile_attributes.bin";
    }

    void TearDown() override
    {
        if (std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::filesystem::path test_dir_;
    std::filesystem::path test_file_;
};

TEST_F(FireRedMetatileAttributeParserTest, ParsesAllFieldsCorrectly)
{
    // Write a single attribute with all fields populated
    {
        std::ofstream out{test_file_};
        // behavior=0x1AB (427), terrain=17, attr_2=9, attr_3=33, encounter=5, attr_5=2, layer=split(2), attr_7=true
        write_firered_attribute(out, 0x1AB, 17, 9, 33, 5, 2, 2, true);
        out.flush();
    }

    auto result = parse_firered_metatile_attributes(test_file_);
    ASSERT_TRUE(result.has_value()) << "Parse failed";
    ASSERT_EQ(result.value().size(), 1);

    const auto &attr = result.value()[0];
    EXPECT_EQ(attr.behavior(), 0x1AB);
    EXPECT_EQ(attr.terrain(), 17);
    EXPECT_EQ(attr.attribute_2(), 9);
    EXPECT_EQ(attr.attribute_3(), 33);
    EXPECT_EQ(attr.encounter_type(), 5);
    EXPECT_EQ(attr.attribute_5(), 2);
    EXPECT_EQ(attr.layer_type(), LayerType::split);
    EXPECT_TRUE(attr.attribute_7());
}

TEST_F(FireRedMetatileAttributeParserTest, ParsesMultipleAttributes)
{
    {
        std::ofstream out{test_file_};
        // Attribute 1: behavior=0, terrain=0, normal layer, all zeros
        write_firered_attribute(out, 0, 0, 0, 0, 0, 0, 0, false);
        // Attribute 2: behavior=511 (max 9 bits), terrain=31 (max 5 bits), covered layer
        write_firered_attribute(out, 511, 31, 15, 63, 7, 3, 1, true);
        out.flush();
    }

    auto result = parse_firered_metatile_attributes(test_file_);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 2);

    // First attribute: all zeros
    const auto &attr0 = result.value()[0];
    EXPECT_EQ(attr0.behavior(), 0);
    EXPECT_EQ(attr0.terrain(), 0);
    EXPECT_EQ(attr0.attribute_2(), 0);
    EXPECT_EQ(attr0.attribute_3(), 0);
    EXPECT_EQ(attr0.encounter_type(), 0);
    EXPECT_EQ(attr0.attribute_5(), 0);
    EXPECT_EQ(attr0.layer_type(), LayerType::normal);
    EXPECT_FALSE(attr0.attribute_7());

    // Second attribute: all max values
    const auto &attr1 = result.value()[1];
    EXPECT_EQ(attr1.behavior(), 511);
    EXPECT_EQ(attr1.terrain(), 31);
    EXPECT_EQ(attr1.attribute_2(), 15);
    EXPECT_EQ(attr1.attribute_3(), 63);
    EXPECT_EQ(attr1.encounter_type(), 7);
    EXPECT_EQ(attr1.attribute_5(), 3);
    EXPECT_EQ(attr1.layer_type(), LayerType::covered);
    EXPECT_TRUE(attr1.attribute_7());
}

TEST_F(FireRedMetatileAttributeParserTest, ErrorOnNonMultipleOf4)
{
    // Write 5 bytes (not a multiple of 4)
    {
        std::ofstream out{test_file_};
        out << '\x00' << '\x00' << '\x00' << '\x00' << '\x01';
        out.flush();
    }

    auto result = parse_firered_metatile_attributes(test_file_);
    ASSERT_FALSE(result.has_value());
}

TEST_F(FireRedMetatileAttributeParserTest, ErrorOnMissingFile)
{
    auto result = parse_firered_metatile_attributes(test_dir_ / "nonexistent.bin");
    ASSERT_FALSE(result.has_value());
}

TEST_F(FireRedMetatileAttributeParserTest, EmptyFileReturnsEmptyVector)
{
    // Write an empty file
    {
        std::ofstream out{test_file_};
        out.flush();
    }

    auto result = parse_firered_metatile_attributes(test_file_);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}
