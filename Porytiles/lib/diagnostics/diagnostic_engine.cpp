#include "diagnostics/diagnostic_engine.hpp"

#ifndef DOCTEST_CONFIG_DISABLE
#include <doctest.h>
#endif // DOCTEST_CONFIG_DISABLE

#include "panic/panic.hpp"

namespace {

std::optional<std::string> construct_flag(const porytiles::diag_level in_flight_level,
                                          const porytiles::diag_templ &templ) {
    if (in_flight_level == porytiles::diag_level::warning) {
        return std::optional{fmt::format("-W{}", templ.name())};
    }
    if (templ.level() == porytiles::diag_level::warning && in_flight_level == porytiles::diag_level::error) {
        return std::optional{fmt::format("-Werror={}", templ.name())};
    }
    return std::nullopt;
}

} // namespace

namespace porytiles {

void diag_engine::enable_all_warnings() {
    for (const auto &diag : all_diag_templ_names()) {
        // Only apply enablement to diagnostics that are default-warnings
        if (const auto &templ = diag_templ_for(diag); templ.level() == diag_level::warning) {
            enable_at_level(diag, diag_level::warning);
        }
    }
}

void diag_engine::disable_all_warnings() {
    all_warnings_disabled_ = true;
}

void diag_engine::upgrade_enabled_warnings_to_errors() {
    for (const auto &diag : all_diag_templ_names()) {
        // Only apply enablement to diagnostics that are default-warnings
        if (const auto &templ = diag_templ_for(diag); templ.level() == diag_level::warning) {
            if (enabled_at_level_.contains(diag)) {
                auto &set = enabled_at_level_.at(diag);
                set.insert(diag_level::error);
            }
        }
    }
}

void diag_engine::enable_at_level(std::string_view diag, diag_level override) {
    // Only allow warns to be overridden for the warning-as-error case
    if (const auto &templ = diag_templ_for(diag); templ.level() != diag_level::warning) {
        panic("cannot change diagnostic enablement level for non-warning diagnostics");
    }

    // Only allow warnings to be upgraded to err or downgraded to warn
    if (override != diag_level::warning && override != diag_level::error) {
        panic(fmt::format("cannot override diagnostic '{}' level to {}", diag, level_to_str(override)));
    }

    if (enabled_at_level_.contains(diag.data())) {
        auto &set = enabled_at_level_.at(diag.data());
        set.insert(override);
    } else {
        enabled_at_level_.insert({std::string{diag}, std::set{override}});
    }
}

void diag_engine::disable_at_level(std::string_view diag, diag_level override) {
    // Only allow warns to be overridden for the warning-as-error case
    if (const auto &templ = diag_templ_for(diag); templ.level() != diag_level::warning) {
        panic("cannot change diagnostic enablement level for non-warning diagnostics");
    }

    // Only allow warnings to be upgraded to err or downgraded to warn
    if (override != diag_level::warning && override != diag_level::error) {
        panic(fmt::format("cannot override diagnostic '{}' level to {}", diag, level_to_str(override)));
    }

    if (enabled_at_level_.contains(diag.data())) {
        auto &set = enabled_at_level_.at(diag.data());
        set.erase(override);
        if (set.empty()) {
            enabled_at_level_.erase(diag.data());
        }
    }
}

std::uint64_t diag_engine::in_flight_count_for_level(diag_level level) const {
    return std::ranges::count(in_flight_diags_, level, &in_flight_diag::level);
}

std::uint64_t diag_engine::count_for(std::string_view diag) const {
    auto diag_str = std::string{diag};
    if (!diag_counts_.contains(diag_str)) {
        return 0;
    }
    return diag_counts_.at(diag_str);
}

const diag_consumer &diag_engine::consumer() const {
    return *consumer_;
}

diag_level diag_engine::compute_level(std::string_view diag) const {
    const auto &templ = diag_templ_for(diag);

    // Only warnings can "change" levels, so short circuit on anything else
    if (templ.level() != diag_level::warning) {
        return templ.level();
    }

    // Return level override if present
    auto diag_str = std::string{diag};
    if (enabled_at_level_.contains(diag_str)) {
        assert_or_panic(!enabled_at_level_.at(diag_str).empty(),
                        fmt::format("enabled_at_level_[{}] - set was empty!", diag_str));
        // Return the highest level present
        return *enabled_at_level_.at(diag_str).rbegin();
    }

    // Default to the default level from the template
    return templ.level();
}

[[nodiscard]] bool diag_engine::is_enabled(std::string_view diag) const {

    /*
     * If this diagnostic is a remark, error, or fatal by default, it is
     * always enabled.
     */
    if (const auto &templ = diag_templ_for(diag); templ.level() == diag_level::remark ||
                                                  templ.level() == diag_level::error ||
                                                  templ.level() == diag_level::fatal) {
        return true;
    }

    /*
     * Highest precedence is global warning disable. If this is specified,
     * all warnings (including warnings which have been upgraded to errors)
     * will be disabled. Any other override setting will be ignored.
     */
    if (all_warnings_disabled_) {
        return false;
    }

    /*
     * Next highest precedence is an explicitly enabled or disabled
     * diagnostic. If the diagnostic is not present in this map, that means
     * it was not set by the user, so it shouldn't be enabled.
     */
    if (const auto diag_str = std::string{diag}; enabled_at_level_.contains(diag_str)) {
        return true;
    }

    // Lowest precedence, if nothing else passed then diagnostic is disabled
    return false;
}

std::string diag_engine::construct_msg_str(const diag_level in_flight_level, const diag_templ &templ,
                                           const std::vector<std::string> &msg) const {
    std::stringstream ss{};

    auto level_prefix = fmt::format("{}: ", level_to_str(in_flight_level));
    const auto style = fmt::emphasis::bold | fg(color_for_level(in_flight_level));

    // If consumer is a tty, style the prefix with the appropriate color.
    if (consumer_->is_a_tty()) {
        level_prefix = fmt::format("{}", styled(level_prefix, style));
    }

    // Dump the level prefix followed by the first line of the message.
    ss << level_prefix << msg.at(0);

    /*
     * Warnings and warnings-as-errors show "[-Wname-of-warning]" at the end
     * of the first line, so the user can easily identify the source of the
     * diagnostic. Handle that formatting here, styling if the consumer is
     * a tty.
     */
    if (const auto flag = construct_flag(in_flight_level, templ); flag.has_value()) {
        ss << " [";
        const auto styled_flag = fmt::format("{}", styled(flag.value(), style));
        consumer_->is_a_tty() ? ss << styled_flag : ss << flag.value();
        ss << "]";
    }

    // Terminate the first line.
    ss << std::endl;

    // Dump the rest of the message lines (if present).
    for (const auto msg_view = msg | std::views::drop(1); const auto &line : msg_view) {
        ss << line << std::endl;
    }

    return ss.str();
}

} // namespace porytiles

#ifndef DOCTEST_CONFIG_DISABLE
TEST_CASE("diag_engine basic -Wall functionality should work") {
    std::size_t zero = 0;
    porytiles::diag_engine engine{std::make_unique<porytiles::vector_consumer>()};

    porytiles::RGBATile tile{};
    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, "foo", "bar", zero, zero);
    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, "foo", "bar", zero, zero);
    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, "foo", "bar", zero, zero);
    engine.report_partner(porytiles::W_COLOR_PRECISION_LOSS, 0, tile, "foo", zero, zero);
    CHECK(engine.consumer().consumed_count() == 0);
    CHECK(engine.in_flight_count_for_level(porytiles::diag_level::warning) == 0);
    CHECK(engine.count_for(porytiles::W_COLOR_PRECISION_LOSS) == 0);

    engine.enable_all_warnings();

    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, "foo", "bar", zero, zero);
    engine.report_partner(porytiles::W_COLOR_PRECISION_LOSS, 0, tile, "foo", zero, zero);
    CHECK(engine.consumer().consumed_count() == 2);
    CHECK(engine.in_flight_count_for_level(porytiles::diag_level::warning) == 1);
    CHECK(engine.in_flight_count_for_level(porytiles::diag_level::note) == 1);
    CHECK(engine.count_for(porytiles::W_COLOR_PRECISION_LOSS) == 1);

    engine.report(porytiles::W_TRANSPARENCY_COLLAPSE, "foo", "bar", "baz", zero, zero);
    CHECK(engine.consumer().consumed_count() == 3);
    CHECK(engine.in_flight_count_for_level(porytiles::diag_level::warning) == 2);
    CHECK(engine.in_flight_count_for_level(porytiles::diag_level::note) == 1);
    CHECK(engine.count_for(porytiles::W_TRANSPARENCY_COLLAPSE) == 1);

    engine.report(porytiles::W_UNUSED_ATTRIBUTE, 12);
    CHECK(engine.consumer().consumed_count() == 4);
    CHECK(engine.in_flight_count_for_level(porytiles::diag_level::warning) == 3);
    CHECK(engine.in_flight_count_for_level(porytiles::diag_level::note) == 1);
    CHECK(engine.count_for(porytiles::W_UNUSED_ATTRIBUTE) == 1);
}

TEST_CASE("diag_engine explicit enablement should work as expected") {
    std::size_t zero = 0;
    porytiles::diag_engine engine{std::make_unique<porytiles::vector_consumer>()};

    porytiles::RGBATile tile{};
    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, "foo", "bar", zero, zero);
    CHECK(engine.consumer().consumed_count() == 0);
    CHECK(engine.in_flight_count_for_level(porytiles::diag_level::warning) == 0);

    engine.enable_at_level(porytiles::W_COLOR_PRECISION_LOSS, porytiles::diag_level::warning);
    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, "foo", "bar", zero, zero);
    CHECK(engine.consumer().consumed_count() == 1);
    CHECK(engine.in_flight_count_for_level(porytiles::diag_level::warning) == 1);

    engine.enable_at_level(porytiles::W_COLOR_PRECISION_LOSS, porytiles::diag_level::error);
    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, "foo", "bar", zero, zero);
    CHECK(engine.consumer().consumed_count() == 2);
    CHECK(engine.in_flight_count_for_level(porytiles::diag_level::warning) == 1);
    CHECK(engine.in_flight_count_for_level(porytiles::diag_level::error) == 1);

    // Disabling at warning doesn't change anything, since it's still enabled at error level
    engine.disable_at_level(porytiles::W_COLOR_PRECISION_LOSS, porytiles::diag_level::warning);
    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, "foo", "bar", zero, zero);
    CHECK(engine.consumer().consumed_count() == 3);
    CHECK(engine.in_flight_count_for_level(porytiles::diag_level::warning) == 1);
    CHECK(engine.in_flight_count_for_level(porytiles::diag_level::error) == 2);

    engine.disable_at_level(porytiles::W_COLOR_PRECISION_LOSS, porytiles::diag_level::error);
    engine.report(porytiles::W_COLOR_PRECISION_LOSS, tile, "foo", "bar", zero, zero);
    CHECK(engine.consumer().consumed_count() == 3);
    CHECK(engine.in_flight_count_for_level(porytiles::diag_level::warning) == 1);
    CHECK(engine.in_flight_count_for_level(porytiles::diag_level::error) == 2);
}
#endif
