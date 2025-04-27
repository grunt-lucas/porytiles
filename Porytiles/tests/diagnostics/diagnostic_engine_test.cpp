#include <gtest/gtest.h>

#include "porytiles/diagnostics/diagnostic_engine.hpp"

TEST(WarningTests, WallShouldEnableAllWarnings) {
    porytiles::DiagEngine engine{std::make_unique<porytiles::IgnoreConsumer>()};

    ASSERT_EQ(engine.enabled_at(porytiles::W_COLOR_PRECISION_LOSS), porytiles::DiagLevel::Ignored);
    ASSERT_EQ(engine.enabled_at(porytiles::W_TRANSPARENCY_COLLAPSE), porytiles::DiagLevel::Ignored);
    ASSERT_EQ(engine.enabled_at(porytiles::W_UNUSED_ATTRIBUTE), porytiles::DiagLevel::Ignored);

    porytiles::RGBATile tile{};
    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    engine.report_partner(porytiles::W_COLOR_PRECISION_LOSS, 0, tile, std::string{"foo"}, 0UL, 0UL);
    ASSERT_EQ(engine.consumer().consumed_count(), 0);
    ASSERT_EQ(engine.in_flight_count_for_level(porytiles::DiagLevel::Warning), 0);
    ASSERT_EQ(engine.in_flight_count_for(porytiles::W_COLOR_PRECISION_LOSS), 0);

    engine.enable_all_warnings();
    ASSERT_EQ(engine.enabled_at(porytiles::W_COLOR_PRECISION_LOSS), porytiles::DiagLevel::Warning);
    ASSERT_EQ(engine.enabled_at(porytiles::W_TRANSPARENCY_COLLAPSE), porytiles::DiagLevel::Warning);
    ASSERT_EQ(engine.enabled_at(porytiles::W_UNUSED_ATTRIBUTE), porytiles::DiagLevel::Warning);

    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    engine.report_partner(porytiles::W_COLOR_PRECISION_LOSS, 0, tile, std::string{"foo"}, 0UL, 0UL);
    ASSERT_EQ(engine.consumer().consumed_count(), 2);
    ASSERT_EQ(engine.in_flight_count_for_level(porytiles::DiagLevel::Warning), 1);
    ASSERT_EQ(engine.in_flight_count_for_level(porytiles::DiagLevel::Note), 1);
    ASSERT_EQ(engine.in_flight_count_for(porytiles::W_COLOR_PRECISION_LOSS), 1);

    engine.report(porytiles::W_TRANSPARENCY_COLLAPSE, "foo", "bar", "baz", 0UL, 0UL);
    ASSERT_EQ(engine.consumer().consumed_count(), 3);
    ASSERT_EQ(engine.in_flight_count_for_level(porytiles::DiagLevel::Warning), 2);
    ASSERT_EQ(engine.in_flight_count_for_level(porytiles::DiagLevel::Note), 1);
    ASSERT_EQ(engine.in_flight_count_for(porytiles::W_TRANSPARENCY_COLLAPSE), 1);

    engine.report(porytiles::W_UNUSED_ATTRIBUTE, 12UL);
    ASSERT_EQ(engine.consumer().consumed_count(), 4);
    ASSERT_EQ(engine.in_flight_count_for_level(porytiles::DiagLevel::Warning), 3);
    ASSERT_EQ(engine.in_flight_count_for_level(porytiles::DiagLevel::Note), 1);
    ASSERT_EQ(engine.in_flight_count_for(porytiles::W_UNUSED_ATTRIBUTE), 1);
}

TEST(WarningTests, IndividualWarningsShouldExplicitlyEnable) {
    porytiles::DiagEngine engine{std::make_unique<porytiles::IgnoreConsumer>()};
    ASSERT_EQ(engine.enabled_at(porytiles::W_COLOR_PRECISION_LOSS), porytiles::DiagLevel::Ignored);

    porytiles::RGBATile tile{};
    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    ASSERT_EQ(engine.consumer().consumed_count(), 0);
    ASSERT_EQ(engine.in_flight_count_for_level(porytiles::DiagLevel::Warning), 0);

    engine.enable_at_level(porytiles::W_COLOR_PRECISION_LOSS, porytiles::DiagLevel::Warning);
    ASSERT_EQ(engine.enabled_at(porytiles::W_COLOR_PRECISION_LOSS), porytiles::DiagLevel::Warning);

    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    ASSERT_EQ(engine.consumer().consumed_count(), 1);
    ASSERT_EQ(engine.in_flight_count_for_level(porytiles::DiagLevel::Warning), 1);

    engine.enable_at_level(porytiles::W_COLOR_PRECISION_LOSS, porytiles::DiagLevel::Error);
    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    ASSERT_EQ(engine.consumer().consumed_count(), 2);
    ASSERT_EQ(engine.in_flight_count_for_level(porytiles::DiagLevel::Warning), 1);
    ASSERT_EQ(engine.in_flight_count_for_level(porytiles::DiagLevel::Error), 1);

    // Disabling at warning doesn't change anything, since it's still enabled at error level
    engine.disable_at_level(porytiles::W_COLOR_PRECISION_LOSS, porytiles::DiagLevel::Warning);
    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    ASSERT_EQ(engine.consumer().consumed_count(), 3);
    ASSERT_EQ(engine.in_flight_count_for_level(porytiles::DiagLevel::Warning), 1);
    ASSERT_EQ(engine.in_flight_count_for_level(porytiles::DiagLevel::Error), 2);

    engine.disable_at_level(porytiles::W_COLOR_PRECISION_LOSS, porytiles::DiagLevel::Error);
    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, std::string{"foo"}, std::string{"bar"}, 0UL, 0UL);
    ASSERT_EQ(engine.consumer().consumed_count(), 3);
    ASSERT_EQ(engine.in_flight_count_for_level(porytiles::DiagLevel::Warning), 1);
    ASSERT_EQ(engine.in_flight_count_for_level(porytiles::DiagLevel::Error), 2);
}
