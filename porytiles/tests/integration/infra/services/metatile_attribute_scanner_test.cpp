#include "porytiles/infra/services/metatile_attribute_scanner.hpp"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>

#include <gtest/gtest.h>

#include "porytiles/utilities/text/plain_text_formatter.hpp"

namespace porytiles {
namespace {

class MetatileAttributeScannerTest : public ::testing::Test {
  protected:
    PlainTextFormatter formatter_;

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
    MetatileAttributeScanner scanner{fixture_base / "emerald", &formatter_};
    const auto outcome = scanner.scan_project();

    ASSERT_TRUE(outcome.fieldmap_present);
    EXPECT_NE(outcome.source.find("global.fieldmap.h"), std::string::npos);
    EXPECT_EQ(define_value(outcome.scan, "METATILE_ATTR_BEHAVIOR_MASK"), 0x00FFU);
    EXPECT_EQ(define_value(outcome.scan, "METATILE_ATTR_LAYER_MASK"), 0xF000U);
    EXPECT_TRUE(outcome.scan.masks_array.empty());
    EXPECT_TRUE(outcome.scan.shifts_array.empty());
    EXPECT_TRUE(outcome.scan.behaviors_header_present);
    EXPECT_EQ(outcome.scan.attributes_element_type, "u16");
    // The unresolvable backslash-continuation define is skipped tolerantly, not warned about.
    EXPECT_TRUE(outcome.warnings.empty());
}

// The firered fixture's exact-name mask/shift tables are read entry by entry, and struct Tileset declares a u32
// pointer.
TEST_F(MetatileAttributeScannerTest, FixtureFireredRawFacts)
{
    MetatileAttributeScanner scanner{fixture_base / "firered", &formatter_};
    const auto outcome = scanner.scan_project();

    ASSERT_TRUE(outcome.fieldmap_present);
    ASSERT_EQ(outcome.scan.masks_array.size(), 8U);
    EXPECT_EQ(outcome.scan.masks_array.front().index_name, "METATILE_ATTRIBUTE_BEHAVIOR");
    EXPECT_EQ(outcome.scan.masks_array.front().value, 0x1FFU);
    ASSERT_EQ(outcome.scan.shifts_array.size(), 8U);
    EXPECT_EQ(outcome.scan.shifts_array.back().index_name, "METATILE_ATTRIBUTE_7");
    EXPECT_EQ(outcome.scan.shifts_array.back().value, 31U);
    EXPECT_TRUE(has_enum_member(outcome.scan, "METATILE_ATTRIBUTE_ENCOUNTER_TYPE"));
    EXPECT_EQ(outcome.scan.attributes_element_type, "u32");
}

// The expansion fixture proves the cross-file seeding and the exact-name rule at the raw-fact level: the table's
// _FRLG macro entries resolve through the header's symbols, and the decoy sMetatileAttrMasksEmerald is ignored.
TEST_F(MetatileAttributeScannerTest, FixtureExpansionSeedsMacrosAndIgnoresDecoyTable)
{
    MetatileAttributeScanner scanner{fixture_base / "expansion", &formatter_};
    const auto outcome = scanner.scan_project();

    ASSERT_TRUE(outcome.fieldmap_present);
    EXPECT_EQ(define_value(outcome.scan, "METATILE_ATTR_BEHAVIOR_MASK_FRLG"), 0x1FFU);
    ASSERT_EQ(outcome.scan.masks_array.size(), 8U);
    EXPECT_EQ(outcome.scan.masks_array.front().index_name, "METATILE_ATTRIBUTE_BEHAVIOR");
    // Seeded from the header's METATILE_ATTR_BEHAVIOR_MASK_FRLG, not the decoy table's 0xFF.
    EXPECT_EQ(outcome.scan.masks_array.front().value, 0x1FFU);
    // No struct Tileset in this fixture, so the element type stays unset.
    EXPECT_FALSE(outcome.scan.attributes_element_type.has_value());
}

// A conflicting redefinition inside an undecidable conditional is returned as data: the name lands in the ambiguous
// set and the recoverable warning rides in the outcome instead of hitting a sink.
TEST_F(MetatileAttributeScannerTest, FixtureAmbiguousReturnsWarningsAsData)
{
    MetatileAttributeScanner scanner{fixture_base / "ambiguous", &formatter_};
    const auto outcome = scanner.scan_project();

    ASSERT_TRUE(outcome.fieldmap_present);
    EXPECT_TRUE(outcome.scan.ambiguous_defines.contains("METATILE_ATTR_BEHAVIOR_MASK"));
    ASSERT_FALSE(outcome.warnings.empty());
    EXPECT_NE(outcome.warnings.front().find("METATILE_ATTR_BEHAVIOR_MASK"), std::string::npos);
}

// A present-but-unparseable src/fieldmap.c is surfaced as a warning, not silently swallowed: the header still parses
// (so the fieldmap is present and its defines are read), but the source table fails to lex, and the scanner says so
// instead of leaving the tables silently empty as if the file were missing.
TEST_F(MetatileAttributeScannerTest, PresentButUnparseableSourceTableWarns)
{
    MetatileAttributeScanner scanner{fixture_base / "bad_source", &formatter_};
    const auto outcome = scanner.scan_project();

    ASSERT_TRUE(outcome.fieldmap_present);
    // Header defines still land: inference continues from them.
    EXPECT_EQ(define_value(outcome.scan, "METATILE_ATTR_BEHAVIOR_MASK"), 0x00FFU);
    // The unparseable table leaves the arrays empty but must not do so quietly.
    EXPECT_TRUE(outcome.scan.masks_array.empty());
    EXPECT_TRUE(outcome.scan.shifts_array.empty());
    const bool warned = std::any_of(outcome.warnings.begin(), outcome.warnings.end(), [](const std::string &warning) {
        return warning.find("fieldmap.c") != std::string::npos;
    });
    EXPECT_TRUE(warned);
}

// A behaviors header declaring its MB_ names as enum members only (stock pokeemerald style, no MB_ defines at all)
// still counts as present: the check accepts either declaration form.
TEST_F(MetatileAttributeScannerTest, FixtureEnumOnlyBehaviorsHeaderIsPresent)
{
    MetatileAttributeScanner scanner{fixture_base / "warns", &formatter_};
    const auto outcome = scanner.scan_project();

    ASSERT_TRUE(outcome.fieldmap_present);
    EXPECT_TRUE(outcome.scan.behaviors_header_present);
}

// A tree with no fieldmap header states nothing: not present, no facts, no warnings.
TEST_F(MetatileAttributeScannerTest, MissingTreeIsNotPresent)
{
    MetatileAttributeScanner scanner{fixture_base / "does_not_exist", &formatter_};
    const auto outcome = scanner.scan_project();

    EXPECT_FALSE(outcome.fieldmap_present);
    EXPECT_TRUE(outcome.scan.defines.empty());
    EXPECT_TRUE(outcome.scan.enum_members.empty());
    EXPECT_TRUE(outcome.warnings.empty());
}

// --- Testbed acceptance skims: run against the local decomp checkouts when they are present. ---

TEST_F(MetatileAttributeScannerTest, PokeemeraldAcceptance)
{
    const std::filesystem::path root = "./pokeemerald";
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "pokeemerald testbed not present";
    }

    MetatileAttributeScanner scanner{root, &formatter_};
    const auto outcome = scanner.scan_project();

    ASSERT_TRUE(outcome.fieldmap_present);
    EXPECT_EQ(define_value(outcome.scan, "METATILE_ATTR_BEHAVIOR_MASK"), 0x00FFU);
    EXPECT_TRUE(outcome.scan.behaviors_header_present);
    EXPECT_EQ(outcome.scan.attributes_element_type, "u16");
}

TEST_F(MetatileAttributeScannerTest, PokefireredAcceptance)
{
    const std::filesystem::path root = "./pokefirered";
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "pokefirered testbed not present";
    }

    MetatileAttributeScanner scanner{root, &formatter_};
    const auto outcome = scanner.scan_project();

    ASSERT_TRUE(outcome.fieldmap_present);
    ASSERT_EQ(outcome.scan.masks_array.size(), 8U);
    EXPECT_TRUE(outcome.scan.behaviors_header_present);
    EXPECT_EQ(outcome.scan.attributes_element_type, "u32");
}

TEST_F(MetatileAttributeScannerTest, PokeemeraldExpansionAcceptance)
{
    const std::filesystem::path root = "./pokeemerald-expansion";
    if (!std::filesystem::exists(root)) {
        GTEST_SKIP() << "pokeemerald-expansion testbed not present";
    }

    MetatileAttributeScanner scanner{root, &formatter_};
    const auto outcome = scanner.scan_project();

    ASSERT_TRUE(outcome.fieldmap_present);
    // Both mask layouts are declared: bare and FRLG defines side by side.
    EXPECT_TRUE(define_value(outcome.scan, "METATILE_ATTR_BEHAVIOR_MASK").has_value());
    EXPECT_TRUE(define_value(outcome.scan, "METATILE_ATTR_BEHAVIOR_MASK_FRLG").has_value());
    EXPECT_EQ(outcome.scan.attributes_element_type, "u16");
}

} // namespace
} // namespace porytiles
