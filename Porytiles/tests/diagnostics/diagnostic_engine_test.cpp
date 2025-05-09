#include <gtest/gtest.h>

#include "porytiles/diagnostics/diagnostic_engine.hpp"

TEST(WarningTests, WallShouldEnableAllWarnings) {
    porytiles::DiagEngine engine{std::make_unique<porytiles::IgnoreConsumer>()};

    ASSERT_EQ(engine.EnabledAt(porytiles::kWarnColorPrecisionLoss), porytiles::DiagLevel::Ignored);
    ASSERT_EQ(engine.EnabledAt(porytiles::kWarnTransparencyCollapse), porytiles::DiagLevel::Ignored);
    ASSERT_EQ(engine.EnabledAt(porytiles::kWarnUnusedAttribute), porytiles::DiagLevel::Ignored);

    porytiles::RGBATile tile{};
    engine.Report(porytiles::kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    engine.Report(porytiles::kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    engine.Report(porytiles::kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    engine.ReportPartner(porytiles::kWarnColorPrecisionLoss, 0, tile, std::string{"foo"}, 0UL, 0UL);
    ASSERT_EQ(engine.consumer().ConsumedCount(), 0);
    ASSERT_EQ(engine.InFlightCountForLevel(porytiles::DiagLevel::Warning), 0);
    ASSERT_EQ(engine.InFlightCountFor(porytiles::kWarnColorPrecisionLoss), 0);

    engine.EnableAllWarnings();
    ASSERT_EQ(engine.EnabledAt(porytiles::kWarnColorPrecisionLoss), porytiles::DiagLevel::Warning);
    ASSERT_EQ(engine.EnabledAt(porytiles::kWarnTransparencyCollapse), porytiles::DiagLevel::Warning);
    ASSERT_EQ(engine.EnabledAt(porytiles::kWarnUnusedAttribute), porytiles::DiagLevel::Warning);

    engine.Report(porytiles::kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    engine.ReportPartner(porytiles::kWarnColorPrecisionLoss, 0, tile, std::string{"foo"}, 0UL, 0UL);
    ASSERT_EQ(engine.consumer().ConsumedCount(), 2);
    ASSERT_EQ(engine.InFlightCountForLevel(porytiles::DiagLevel::Warning), 1);
    ASSERT_EQ(engine.InFlightCountForLevel(porytiles::DiagLevel::Note), 1);
    ASSERT_EQ(engine.InFlightCountFor(porytiles::kWarnColorPrecisionLoss), 1);

    engine.Report(porytiles::kWarnTransparencyCollapse, "foo", "bar", "baz", 0UL, 0UL);
    ASSERT_EQ(engine.consumer().ConsumedCount(), 3);
    ASSERT_EQ(engine.InFlightCountForLevel(porytiles::DiagLevel::Warning), 2);
    ASSERT_EQ(engine.InFlightCountForLevel(porytiles::DiagLevel::Note), 1);
    ASSERT_EQ(engine.InFlightCountFor(porytiles::kWarnTransparencyCollapse), 1);

    engine.Report(porytiles::kWarnUnusedAttribute, 12UL);
    ASSERT_EQ(engine.consumer().ConsumedCount(), 4);
    ASSERT_EQ(engine.InFlightCountForLevel(porytiles::DiagLevel::Warning), 3);
    ASSERT_EQ(engine.InFlightCountForLevel(porytiles::DiagLevel::Note), 1);
    ASSERT_EQ(engine.InFlightCountFor(porytiles::kWarnUnusedAttribute), 1);
}

TEST(WarningTests, IndividualWarningsShouldExplicitlyEnable) {
    porytiles::DiagEngine engine{std::make_unique<porytiles::IgnoreConsumer>()};
    ASSERT_EQ(engine.EnabledAt(porytiles::kWarnColorPrecisionLoss), porytiles::DiagLevel::Ignored);

    porytiles::RGBATile tile{};
    engine.Report(porytiles::kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    ASSERT_EQ(engine.consumer().ConsumedCount(), 0);
    ASSERT_EQ(engine.InFlightCountForLevel(porytiles::DiagLevel::Warning), 0);

    engine.EnableAtLevel(porytiles::kWarnColorPrecisionLoss, porytiles::DiagLevel::Warning);
    ASSERT_EQ(engine.EnabledAt(porytiles::kWarnColorPrecisionLoss), porytiles::DiagLevel::Warning);

    engine.Report(porytiles::kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    ASSERT_EQ(engine.consumer().ConsumedCount(), 1);
    ASSERT_EQ(engine.InFlightCountForLevel(porytiles::DiagLevel::Warning), 1);

    engine.EnableAtLevel(porytiles::kWarnColorPrecisionLoss, porytiles::DiagLevel::Error);
    engine.Report(porytiles::kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    ASSERT_EQ(engine.consumer().ConsumedCount(), 2);
    ASSERT_EQ(engine.InFlightCountForLevel(porytiles::DiagLevel::Warning), 1);
    ASSERT_EQ(engine.InFlightCountForLevel(porytiles::DiagLevel::Error), 1);

    // Disabling at warning doesn't change anything, since it's still enabled at error level
    engine.DisableAtLevel(porytiles::kWarnColorPrecisionLoss, porytiles::DiagLevel::Warning);
    engine.Report(porytiles::kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    ASSERT_EQ(engine.consumer().ConsumedCount(), 3);
    ASSERT_EQ(engine.InFlightCountForLevel(porytiles::DiagLevel::Warning), 1);
    ASSERT_EQ(engine.InFlightCountForLevel(porytiles::DiagLevel::Error), 2);

    engine.DisableAtLevel(porytiles::kWarnColorPrecisionLoss, porytiles::DiagLevel::Error);
    engine.Report(porytiles::kWarnColorPrecisionLoss, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    ASSERT_EQ(engine.consumer().ConsumedCount(), 3);
    ASSERT_EQ(engine.InFlightCountForLevel(porytiles::DiagLevel::Warning), 1);
    ASSERT_EQ(engine.InFlightCountForLevel(porytiles::DiagLevel::Error), 2);
}
