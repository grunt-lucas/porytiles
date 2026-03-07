#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

#include "porytiles2/domain/models/base_game.hpp"
#include "porytiles2/infra/services/base_game_detector.hpp"
#include "porytiles2/utilities/text/plain_text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/buffered_user_diagnostics.hpp"

using namespace porytiles2;

class BaseGameDetectorTest : public ::testing::Test {
  protected:
    void SetUp() override
    {
        test_root_ = std::filesystem::temp_directory_path() / "porytiles_base_game_detector_test";
        std::filesystem::create_directories(test_root_ / "include");
    }

    void TearDown() override
    {
        if (std::filesystem::exists(test_root_)) {
            std::filesystem::remove_all(test_root_);
        }
    }

    std::filesystem::path test_root_;
    PlainTextFormatter formatter_;
    BufferedUserDiagnostics diag_;
};

TEST_F(BaseGameDetectorTest, DetectsPokefirered)
{
    {
        std::ofstream out{test_root_ / "include/global.fieldmap.h"};
        out << "#ifndef GUARD_GLOBAL_FIELDMAP_H\n";
        out << "#define GUARD_GLOBAL_FIELDMAP_H\n";
        out << "enum {\n";
        out << "    METATILE_ATTRIBUTE_BEHAVIOR = 0x000001FF,\n";
        out << "};\n";
        out << "#endif\n";
        out.flush();
    }

    BaseGameDetector detector{test_root_, &formatter_, &diag_};
    auto result = detector.detect();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), BaseGame::pokefirered);
}

TEST_F(BaseGameDetectorTest, DetectsPokeemeraldExpansion)
{
    {
        std::ofstream out{test_root_ / "include/global.fieldmap.h"};
        out << "#ifndef GUARD_GLOBAL_FIELDMAP_H\n";
        out << "#define GUARD_GLOBAL_FIELDMAP_H\n";
        out << "#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF\n";
        out << "#define MAPGRID_METATILE_ID_SHIFT 0\n";
        out << "void swapPalettes(void);\n";
        out << "#endif\n";
        out.flush();
    }

    BaseGameDetector detector{test_root_, &formatter_, &diag_};
    auto result = detector.detect();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), BaseGame::pokeemerald_expansion);
}

TEST_F(BaseGameDetectorTest, DetectsPokeemerald)
{
    {
        std::ofstream out{test_root_ / "include/global.fieldmap.h"};
        out << "#ifndef GUARD_GLOBAL_FIELDMAP_H\n";
        out << "#define GUARD_GLOBAL_FIELDMAP_H\n";
        out << "#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF\n";
        out << "#define MAPGRID_METATILE_ID_SHIFT 0\n";
        out << "#endif\n";
        out.flush();
    }

    BaseGameDetector detector{test_root_, &formatter_, &diag_};
    auto result = detector.detect();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), BaseGame::pokeemerald);
}

TEST_F(BaseGameDetectorTest, DetectsPokeruby)
{
    {
        std::ofstream out{test_root_ / "include/global.fieldmap.h"};
        out << "#ifndef GUARD_GLOBAL_FIELDMAP_H\n";
        out << "#define GUARD_GLOBAL_FIELDMAP_H\n";
        out << "#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF\n";
        out << "#endif\n";
        out.flush();
    }

    BaseGameDetector detector{test_root_, &formatter_, &diag_};
    auto result = detector.detect();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), BaseGame::pokeruby);
}

TEST_F(BaseGameDetectorTest, ErrorWhenNoMarkersFound)
{
    {
        std::ofstream out{test_root_ / "include/global.fieldmap.h"};
        out << "#ifndef GUARD_GLOBAL_FIELDMAP_H\n";
        out << "#define GUARD_GLOBAL_FIELDMAP_H\n";
        out << "// Empty header with no recognizable markers\n";
        out << "#endif\n";
        out.flush();
    }

    BaseGameDetector detector{test_root_, &formatter_, &diag_};
    auto result = detector.detect();
    EXPECT_FALSE(result.has_value());
}

TEST_F(BaseGameDetectorTest, ErrorWhenFileMissing)
{
    std::filesystem::remove(test_root_ / "include/global.fieldmap.h");

    BaseGameDetector detector{test_root_, &formatter_, &diag_};
    auto result = detector.detect();
    EXPECT_FALSE(result.has_value());
}

TEST_F(BaseGameDetectorTest, EmitsRemarkOnDetection)
{
    {
        std::ofstream out{test_root_ / "include/global.fieldmap.h"};
        out << "#define METATILE_ATTR_BEHAVIOR_MASK 0x00FF\n";
        out << "#define MAPGRID_METATILE_ID_SHIFT 0\n";
        out.flush();
    }

    BaseGameDetector detector{test_root_, &formatter_, &diag_};
    auto result = detector.detect();
    ASSERT_TRUE(result.has_value());

    const auto &tag_counts = diag_.remark_tag_counts();
    auto it = tag_counts.find("base-game-detection");
    ASSERT_NE(it, tag_counts.end());
    EXPECT_EQ(it->second, 1u);

    const auto &note_tag_counts = diag_.remark_note_tag_counts();
    auto note_it = note_tag_counts.find("base-game-detection");
    ASSERT_NE(note_it, note_tag_counts.end());
    EXPECT_EQ(note_it->second, 1u);
}
