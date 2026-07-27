#include "porytiles/infra/services/metatile_attribute_scanner.hpp"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "porytiles/utilities/text/plain_text_formatter.hpp"
#include "porytiles/xcut/diagnostics/buffered_user_diagnostics.hpp"

namespace porytiles {
namespace {

class MetatileAttributeScannerTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;
    BufferedUserDiagnostics diag_;

    // The buffered warnings flattened to one line per emitted line, so tests can probe their text directly.
    [[nodiscard]] std::vector<std::string> warning_lines() const
    {
        std::vector<std::string> lines;
        for (const auto &warning : diag_.warnings()) {
            lines.insert(lines.end(), warning.begin(), warning.end());
        }
        return lines;
    }

    [[nodiscard]] static std::optional<std::uint32_t>
    define_value(const MetatileAttributeScan &scan, const std::string &name)
    {
        const auto it = std::find_if(scan.defines.begin(), scan.defines.end(), [&](const InferenceDefine &define) {
            return define.name == name;
        });
        if (it == scan.defines.end()) {
            return std::nullopt;
        }
        return it->value;
    }

    [[nodiscard]] static bool has_enum_member(const MetatileAttributeScan &scan, const std::string &name)
    {
        return std::any_of(scan.enum_members.begin(), scan.enum_members.end(), [&](const InferenceEnumMember &member) {
            return member.name == name;
        });
    }
};

const std::filesystem::path fixture_base = "resources/tests/integration/infra/services/metatile_attribute_scanner";

// The scanner reports raw facts only: the emerald fixture's mask defines land in the define list verbatim, the
// struct member's element type is captured as the raw type name, and the absent mask table stays empty.
TEST_F(MetatileAttributeScannerTest, FixtureEmeraldRawFacts)
{
    MetatileAttributeScanner scanner{fixture_base / "emerald", &formatter_, &diag_};
    const auto scan = scanner.scan_project();

    EXPECT_NE(scan.header_source.find("global.fieldmap.h"), std::string::npos);
    EXPECT_EQ(define_value(scan, "METATILE_ATTR_BEHAVIOR_MASK"), 0x00FFU);
    EXPECT_EQ(define_value(scan, "METATILE_ATTR_LAYER_MASK"), 0xF000U);
    EXPECT_TRUE(scan.masks_array.empty());
    EXPECT_TRUE(scan.shifts_array.empty());
    // No mask table was read, so no path claims one.
    EXPECT_TRUE(scan.masks_table_source.empty());
    EXPECT_TRUE(scan.behaviors_header_present);
    EXPECT_EQ(scan.attributes_element_type, "u16");
    // The unresolvable backslash-continuation define is skipped tolerantly, not warned about.
    EXPECT_TRUE(diag_.warnings().empty());
}

// The firered fixture's exact-name mask/shift tables are read entry by entry, and struct Tileset declares a u32
// pointer.
TEST_F(MetatileAttributeScannerTest, FixtureFireredRawFacts)
{
    MetatileAttributeScanner scanner{fixture_base / "firered", &formatter_, &diag_};
    const auto scan = scanner.scan_project();

    ASSERT_EQ(scan.masks_array.size(), 8U);
    EXPECT_EQ(scan.masks_array.front().index_name, "METATILE_ATTRIBUTE_BEHAVIOR");
    EXPECT_EQ(scan.masks_array.front().value, 0x1FFU);
    ASSERT_EQ(scan.shifts_array.size(), 8U);
    EXPECT_EQ(scan.shifts_array.back().index_name, "METATILE_ATTRIBUTE_7");
    EXPECT_EQ(scan.shifts_array.back().value, 31U);
    EXPECT_TRUE(has_enum_member(scan, "METATILE_ATTRIBUTE_ENCOUNTER_TYPE"));
    EXPECT_EQ(scan.attributes_element_type, "u32");
    // The masks came from the source file, not the header, and the recorded path says so.
    EXPECT_NE(scan.masks_table_source.find("fieldmap.c"), std::string::npos);
    EXPECT_NE(scan.header_source.find("global.fieldmap.h"), std::string::npos);
}

// The expansion fixture proves the cross-file seeding and the exact-name rule at the raw-fact level: the table's
// _FRLG macro entries resolve through the header's symbols, and the decoy sMetatileAttrMasksEmerald is ignored.
TEST_F(MetatileAttributeScannerTest, FixtureExpansionSeedsMacrosAndIgnoresDecoyTable)
{
    MetatileAttributeScanner scanner{fixture_base / "expansion", &formatter_, &diag_};
    const auto scan = scanner.scan_project();

    EXPECT_EQ(define_value(scan, "METATILE_ATTR_BEHAVIOR_MASK_FRLG"), 0x1FFU);
    ASSERT_EQ(scan.masks_array.size(), 8U);
    EXPECT_EQ(scan.masks_array.front().index_name, "METATILE_ATTRIBUTE_BEHAVIOR");
    // Seeded from the header's METATILE_ATTR_BEHAVIOR_MASK_FRLG, not the decoy table's 0xFF.
    EXPECT_EQ(scan.masks_array.front().value, 0x1FFU);
    // No struct Tileset in this fixture, so the element type stays unset.
    EXPECT_FALSE(scan.attributes_element_type.has_value());
}

// A conflicting redefinition inside an undecidable conditional lands in the ambiguous set as a raw fact, and the
// recoverable warning about it reaches the diagnostics sink under the inference tag.
TEST_F(MetatileAttributeScannerTest, FixtureAmbiguousWarnsAboutTheConflictingRedefinition)
{
    MetatileAttributeScanner scanner{fixture_base / "ambiguous", &formatter_, &diag_};
    const auto scan = scanner.scan_project();

    EXPECT_TRUE(scan.ambiguous_defines.contains("METATILE_ATTR_BEHAVIOR_MASK"));
    const auto lines = warning_lines();
    ASSERT_FALSE(lines.empty());
    EXPECT_EQ(diag_.warning_tag_counts().at(metatile_attr_inference_tag), diag_.warnings().size());
    EXPECT_NE(lines.front().find("METATILE_ATTR_BEHAVIOR_MASK"), std::string::npos);
}

// A present-but-unparseable src/fieldmap.c is surfaced as a warning, not silently swallowed: the header still parses
// (so its defines are read), but the source table fails to lex, and the scanner says so instead of leaving the tables
// silently empty as if the file were missing.
TEST_F(MetatileAttributeScannerTest, PresentButUnparseableSourceTableWarns)
{
    MetatileAttributeScanner scanner{fixture_base / "bad_source", &formatter_, &diag_};
    const auto scan = scanner.scan_project();

    // Header defines still land: inference continues from them.
    EXPECT_EQ(define_value(scan, "METATILE_ATTR_BEHAVIOR_MASK"), 0x00FFU);
    // The unparseable table leaves the arrays empty but must not do so quietly.
    EXPECT_TRUE(scan.masks_array.empty());
    EXPECT_TRUE(scan.shifts_array.empty());
    // The file exists but yielded no masks, so it is not recorded as a mask source.
    EXPECT_TRUE(scan.masks_table_source.empty());
    const auto lines = warning_lines();
    const bool warned = std::any_of(lines.begin(), lines.end(), [](const std::string &line) {
        return line.find("fieldmap.c") != std::string::npos;
    });
    EXPECT_TRUE(warned);
    // The warning can be filtered out, so the fact is also carried in the scan for inference to reason about.
    ASSERT_EQ(scan.unreadable_sources.size(), 1U);
    EXPECT_NE(scan.unreadable_sources.front().find("fieldmap.c"), std::string::npos);
}

// A present-but-unparseable include/global.fieldmap.h is recorded as unreadable, not flattened into "absent". The
// distinction is what keeps the project from being told it declares no attribute masks when it plainly declares
// several: they just could not be read.
TEST_F(MetatileAttributeScannerTest, PresentButUnparseableHeaderIsRecordedAsUnreadable)
{
    MetatileAttributeScanner scanner{fixture_base / "bad_header", &formatter_, &diag_};
    const auto scan = scanner.scan_project();

    EXPECT_TRUE(scan.defines.empty());
    ASSERT_EQ(scan.unreadable_sources.size(), 1U);
    EXPECT_NE(scan.unreadable_sources.front().find("global.fieldmap.h"), std::string::npos);
    const auto lines = warning_lines();
    const bool warned = std::any_of(lines.begin(), lines.end(), [](const std::string &line) {
        return line.find("global.fieldmap.h") != std::string::npos;
    });
    EXPECT_TRUE(warned);
}

// A behaviors header declaring its MB_ names as enum members only (stock pokeemerald style, no MB_ defines at all)
// still counts as present: the check accepts either declaration form.
TEST_F(MetatileAttributeScannerTest, FixtureEnumOnlyBehaviorsHeaderIsPresent)
{
    MetatileAttributeScanner scanner{fixture_base / "warns", &formatter_, &diag_};
    const auto scan = scanner.scan_project();

    EXPECT_TRUE(scan.behaviors_header_present);
}

// A tree with no fieldmap header states nothing: no facts, nothing recorded as unreadable, no warnings. The empty
// unreadable list is what separates this from a header that exists but could not be read.
TEST_F(MetatileAttributeScannerTest, MissingTreeStatesNothing)
{
    MetatileAttributeScanner scanner{fixture_base / "does_not_exist", &formatter_, &diag_};
    const auto scan = scanner.scan_project();

    EXPECT_TRUE(scan.defines.empty());
    EXPECT_TRUE(scan.enum_members.empty());
    EXPECT_TRUE(scan.unreadable_sources.empty());
    EXPECT_TRUE(diag_.warnings().empty());
}

// --- Testbed acceptance skims: run against the local decomp checkouts when they are present. ---

TEST_F(MetatileAttributeScannerTest, PokeemeraldAcceptance)
{
    const std::filesystem::path root = "./pokeemerald";
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "pokeemerald testbed not present";
    }

    MetatileAttributeScanner scanner{root, &formatter_, &diag_};
    const auto scan = scanner.scan_project();

    EXPECT_EQ(define_value(scan, "METATILE_ATTR_BEHAVIOR_MASK"), 0x00FFU);
    EXPECT_TRUE(scan.behaviors_header_present);
    EXPECT_EQ(scan.attributes_element_type, "u16");
}

TEST_F(MetatileAttributeScannerTest, PokefireredAcceptance)
{
    const std::filesystem::path root = "./pokefirered";
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "pokefirered testbed not present";
    }

    MetatileAttributeScanner scanner{root, &formatter_, &diag_};
    const auto scan = scanner.scan_project();

    ASSERT_EQ(scan.masks_array.size(), 8U);
    EXPECT_TRUE(scan.behaviors_header_present);
    EXPECT_EQ(scan.attributes_element_type, "u32");
    EXPECT_EQ(scan.masks_table_source, (root / "src" / "fieldmap.c").string());
}

TEST_F(MetatileAttributeScannerTest, PokeemeraldExpansionAcceptance)
{
    const std::filesystem::path root = "./pokeemerald-expansion";
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "pokeemerald-expansion testbed not present";
    }

    MetatileAttributeScanner scanner{root, &formatter_, &diag_};
    const auto scan = scanner.scan_project();

    // Both mask layouts are declared: bare and FRLG defines side by side.
    EXPECT_TRUE(define_value(scan, "METATILE_ATTR_BEHAVIOR_MASK").has_value());
    EXPECT_TRUE(define_value(scan, "METATILE_ATTR_BEHAVIOR_MASK_FRLG").has_value());
    EXPECT_EQ(scan.attributes_element_type, "u16");
}

} // namespace
} // namespace porytiles
