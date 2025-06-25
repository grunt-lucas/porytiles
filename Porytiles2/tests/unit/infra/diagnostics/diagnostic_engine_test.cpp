#include "gtest/gtest.h"

#include "porytiles2/infra/diagnostics/DiagnosticEngine.hpp"

using namespace porytiles;

TEST(WarningTests, WallShouldEnableAllWarnings) {
    DiagEngine engine{std::make_unique<IgnoreConsumer>()};

    EXPECT_EQ(engine.EnabledAt(WarnColorPrecisionLoss), DiagLevel::kIgnored);
    EXPECT_EQ(engine.EnabledAt(WarnTransparencyCollapse), DiagLevel::kIgnored);
    EXPECT_EQ(engine.EnabledAt(WarnUnusedAttribute), DiagLevel::kIgnored);

    // RGBATile tile{};
    // engine.Report(kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    // engine.Report(kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    // engine.Report(kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    // engine.ReportPartner(kWarnColorPrecisionLoss, 0, tile, std::string{"foo"}, 0UL, 0UL);
    // ASSERT_EQ(engine.consumer().ConsumedCount(), 0);
    // ASSERT_EQ(engine.InFlightCountForLevel(DiagLevel::Warning), 0);
    // ASSERT_EQ(engine.InFlightCountFor(kWarnColorPrecisionLoss), 0);

    // engine.EnableAllWarnings();
    // ASSERT_EQ(engine.EnabledAt(kWarnColorPrecisionLoss), DiagLevel::Warning);
    // ASSERT_EQ(engine.EnabledAt(kWarnTransparencyCollapse), DiagLevel::Warning);
    // ASSERT_EQ(engine.EnabledAt(kWarnUnusedAttribute), DiagLevel::Warning);

    // engine.Report(kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    // engine.ReportPartner(kWarnColorPrecisionLoss, 0, tile, std::string{"foo"}, 0UL, 0UL);
    // ASSERT_EQ(engine.consumer().ConsumedCount(), 2);
    // ASSERT_EQ(engine.InFlightCountForLevel(DiagLevel::Warning), 1);
    // ASSERT_EQ(engine.InFlightCountForLevel(DiagLevel::Note), 1);
    // ASSERT_EQ(engine.InFlightCountFor(kWarnColorPrecisionLoss), 1);

    // engine.Report(kWarnTransparencyCollapse, "foo", "bar", "baz", 0UL, 0UL);
    // ASSERT_EQ(engine.consumer().ConsumedCount(), 3);
    // ASSERT_EQ(engine.InFlightCountForLevel(DiagLevel::Warning), 2);
    // ASSERT_EQ(engine.InFlightCountForLevel(DiagLevel::Note), 1);
    // ASSERT_EQ(engine.InFlightCountFor(kWarnTransparencyCollapse), 1);

    // engine.Report(kWarnUnusedAttribute, 12UL);
    // ASSERT_EQ(engine.consumer().ConsumedCount(), 4);
    // ASSERT_EQ(engine.InFlightCountForLevel(DiagLevel::Warning), 3);
    // ASSERT_EQ(engine.InFlightCountForLevel(DiagLevel::Note), 1);
    // ASSERT_EQ(engine.InFlightCountFor(kWarnUnusedAttribute), 1);
}

TEST(WarningTests, IndividualWarningsShouldExplicitlyEnable) {
    DiagEngine engine{std::make_unique<IgnoreConsumer>()};
    EXPECT_EQ(engine.EnabledAt(WarnColorPrecisionLoss), DiagLevel::kIgnored);

    // RGBATile tile{};
    // engine.Report(kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    // ASSERT_EQ(engine.consumer().ConsumedCount(), 0);
    // ASSERT_EQ(engine.InFlightCountForLevel(DiagLevel::Warning), 0);

    // engine.EnableAtLevel(kWarnColorPrecisionLoss, DiagLevel::Warning);
    // ASSERT_EQ(engine.EnabledAt(kWarnColorPrecisionLoss), DiagLevel::Warning);

    // engine.Report(kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    // ASSERT_EQ(engine.consumer().ConsumedCount(), 1);
    // ASSERT_EQ(engine.InFlightCountForLevel(DiagLevel::Warning), 1);

    // engine.EnableAtLevel(kWarnColorPrecisionLoss, DiagLevel::Error);
    // engine.Report(kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    // ASSERT_EQ(engine.consumer().ConsumedCount(), 2);
    // ASSERT_EQ(engine.InFlightCountForLevel(DiagLevel::Warning), 1);
    // ASSERT_EQ(engine.InFlightCountForLevel(DiagLevel::Error), 1);

    // // Disabling at warning doesn't change anything, since it's still enabled at error level
    // engine.DisableAtLevel(kWarnColorPrecisionLoss, DiagLevel::Warning);
    // engine.Report(kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    // ASSERT_EQ(engine.consumer().ConsumedCount(), 3);
    // ASSERT_EQ(engine.InFlightCountForLevel(DiagLevel::Warning), 1);
    // ASSERT_EQ(engine.InFlightCountForLevel(DiagLevel::Error), 2);

    // engine.DisableAtLevel(kWarnColorPrecisionLoss, DiagLevel::Error);
    // engine.Report(kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    // ASSERT_EQ(engine.consumer().ConsumedCount(), 3);
    // ASSERT_EQ(engine.InFlightCountForLevel(DiagLevel::Warning), 1);
    // ASSERT_EQ(engine.InFlightCountForLevel(DiagLevel::Error), 2);
}
