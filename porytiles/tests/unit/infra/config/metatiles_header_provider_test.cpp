#include "gtest/gtest.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>

#include "porytiles/infra/config/layer_value.hpp"
#include "porytiles/infra/config/metatiles_header_provider.hpp"
#include "porytiles/utilities/text/plain_text_formatter.hpp"

using namespace porytiles;

namespace {

std::filesystem::path create_test_project(const std::string &test_name, const std::string &metatiles_content)
{
    const auto temp_dir =
        std::filesystem::temp_directory_path() / "porytiles_metatiles_header_provider_test" / test_name;
    std::filesystem::remove_all(temp_dir);

    const auto metatiles_dir = temp_dir / "src" / "data" / "tilesets";
    std::filesystem::create_directories(metatiles_dir);

    std::ofstream out{metatiles_dir / "metatiles.h"};
    out << metatiles_content;
    out.close();

    return temp_dir;
}

} // namespace

class MetatilesHeaderProviderTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        formatter_ = std::make_unique<PlainTextFormatter>();
    }

    void TearDown() override
    {
        if (!temp_dir_.empty()) {
            std::filesystem::remove_all(temp_dir_);
        }
    }

    std::filesystem::path temp_dir_;
    std::unique_ptr<PlainTextFormatter> formatter_;
};

TEST_F(MetatilesHeaderProviderTest, DetectsU16Attributes)
{
    temp_dir_ = create_test_project(
        "u16",
        "const u16 gMetatileAttributes_General[] = INCBIN_U16(\"data/tilesets/primary/general/"
        "metatile_attributes.bin\");\n"
        "const u16 gMetatileAttributes_Petalburg[] = INCBIN_U16(\"data/tilesets/secondary/petalburg/"
        "metatile_attributes.bin\");\n");

    MetatilesHeaderProvider provider{temp_dir_, formatter_.get()};
    auto result = provider.detect();

    ASSERT_EQ(result.state, ValidationState::valid);
    ASSERT_TRUE(result.value.has_value());
    EXPECT_EQ(result.value.value(), 2);
}

TEST_F(MetatilesHeaderProviderTest, DetectsU32Attributes)
{
    temp_dir_ = create_test_project(
        "u32",
        "const u32 gMetatileAttributes_General[] = INCBIN_U32(\"data/tilesets/primary/general/"
        "metatile_attributes.bin\");\n"
        "const u32 gMetatileAttributes_Petalburg[] = INCBIN_U32(\"data/tilesets/secondary/petalburg/"
        "metatile_attributes.bin\");\n");

    MetatilesHeaderProvider provider{temp_dir_, formatter_.get()};
    auto result = provider.detect();

    ASSERT_EQ(result.state, ValidationState::valid);
    ASSERT_TRUE(result.value.has_value());
    EXPECT_EQ(result.value.value(), 4);
}

TEST_F(MetatilesHeaderProviderTest, DetectsU8Attributes)
{
    temp_dir_ = create_test_project(
        "u8",
        "const u8 gMetatileAttributes_General[] = INCBIN_U8(\"data/tilesets/primary/general/"
        "metatile_attributes.bin\");\n"
        "const u8 gMetatileAttributes_Petalburg[] = INCBIN_U8(\"data/tilesets/secondary/petalburg/"
        "metatile_attributes.bin\");\n");

    MetatilesHeaderProvider provider{temp_dir_, formatter_.get()};
    auto result = provider.detect();

    ASSERT_EQ(result.state, ValidationState::valid);
    ASSERT_TRUE(result.value.has_value());
    EXPECT_EQ(result.value.value(), 1);
}

TEST_F(MetatilesHeaderProviderTest, MixedU8AndU16ReturnsInvalid)
{
    temp_dir_ = create_test_project(
        "mixed_u8_u16",
        "const u8 gMetatileAttributes_General[] = INCBIN_U8(\"data/tilesets/primary/general/"
        "metatile_attributes.bin\");\n"
        "const u16 gMetatileAttributes_Petalburg[] = INCBIN_U16(\"data/tilesets/secondary/petalburg/"
        "metatile_attributes.bin\");\n");

    MetatilesHeaderProvider provider{temp_dir_, formatter_.get()};
    auto result = provider.detect();

    EXPECT_EQ(result.state, ValidationState::invalid);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(MetatilesHeaderProviderTest, MissingFileReturnsNotProvided)
{
    temp_dir_ = std::filesystem::temp_directory_path() / "porytiles_metatiles_header_provider_test" / "missing";
    std::filesystem::remove_all(temp_dir_);
    std::filesystem::create_directories(temp_dir_);

    MetatilesHeaderProvider provider{temp_dir_, formatter_.get()};
    auto result = provider.detect();

    EXPECT_EQ(result.state, ValidationState::not_provided);
}

TEST_F(MetatilesHeaderProviderTest, NoAttributeLinesReturnsNotProvided)
{
    temp_dir_ = create_test_project(
        "no_attrs", "const u16 gMetatiles_General[] = INCBIN_U16(\"data/tilesets/primary/general/metatiles.bin\");\n");

    MetatilesHeaderProvider provider{temp_dir_, formatter_.get()};
    auto result = provider.detect();

    EXPECT_EQ(result.state, ValidationState::not_provided);
}

TEST_F(MetatilesHeaderProviderTest, MixedTypesReturnsInvalid)
{
    temp_dir_ = create_test_project(
        "mixed",
        "const u16 gMetatileAttributes_General[] = INCBIN_U16(\"data/tilesets/primary/general/"
        "metatile_attributes.bin\");\n"
        "const u32 gMetatileAttributes_Petalburg[] = INCBIN_U32(\"data/tilesets/secondary/petalburg/"
        "metatile_attributes.bin\");\n");

    MetatilesHeaderProvider provider{temp_dir_, formatter_.get()};
    auto result = provider.detect();

    EXPECT_EQ(result.state, ValidationState::invalid);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(MetatilesHeaderProviderTest, CachesResultAcrossCalls)
{
    temp_dir_ = create_test_project(
        "cached",
        "const u16 gMetatileAttributes_General[] = INCBIN_U16(\"data/tilesets/primary/general/"
        "metatile_attributes.bin\");\n");

    MetatilesHeaderProvider provider{temp_dir_, formatter_.get()};

    auto result1 = provider.detect();
    auto result2 = provider.detect();

    // Both calls should return the same cached result
    ASSERT_EQ(result1.state, ValidationState::valid);
    ASSERT_EQ(result2.state, ValidationState::valid);
    EXPECT_EQ(result1.value.value(), result2.value.value());
}
