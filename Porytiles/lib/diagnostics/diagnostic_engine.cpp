#include "diagnostics/diagnostic_engine.hpp"

#ifndef DOCTEST_CONFIG_DISABLE
#include <doctest.h>
#endif // DOCTEST_CONFIG_DISABLE

#include "panic/panic.hpp"

namespace {

std::optional<std::string> construct_flag(const porytiles::DiagLevel in_flight_level,
                                          const porytiles::DiagTempl &templ) {
    if (in_flight_level == porytiles::DiagLevel::Warning) {
        return std::optional{fmt::format("-W{}", templ.name())};
    }
    if (templ.level() == porytiles::DiagLevel::Warning && in_flight_level == porytiles::DiagLevel::Error) {
        return std::optional{fmt::format("-Werror={}", templ.name())};
    }
    return std::nullopt;
}

} // namespace

namespace porytiles {

void DiagEngine::enable_all_warnings() {
    for (const auto &diag : all_diag_templ_names()) {
        // Only apply enablement to diagnostics that are default-warnings
        if (const auto &templ = diag_templ_for(diag); templ.level() == DiagLevel::Warning) {
            enable_at_level(diag, DiagLevel::Warning);
        }
    }
}

void DiagEngine::disable_all_warnings() {
    all_warnings_disabled_ = true;
}

void DiagEngine::upgrade_enabled_warnings_to_errors() {
    for (const auto &diag : all_diag_templ_names()) {
        // Only apply enablement to diagnostics that are default-warnings
        if (const auto &templ = diag_templ_for(diag); templ.level() == DiagLevel::Warning) {
            if (enabled_at_level_.contains(diag)) {
                auto &set = enabled_at_level_.at(diag);
                set.insert(DiagLevel::Error);
            }
        }
    }
}

void DiagEngine::enable_at_level(std::string_view diag, DiagLevel override) {
    // Only allow warns to be overridden for the warning-as-error case
    if (const auto &templ = diag_templ_for(diag); templ.level() != DiagLevel::Warning) {
        panic("cannot change diagnostic enablement level for non-warning diagnostics");
    }

    // Only allow warnings to be upgraded to err or downgraded to warn
    if (override != DiagLevel::Warning && override != DiagLevel::Error) {
        panic(fmt::format("cannot override diagnostic '{}' level to {}", diag, level_to_str(override)));
    }

    if (enabled_at_level_.contains(diag.data())) {
        auto &set = enabled_at_level_.at(diag.data());
        set.insert(override);
    } else {
        enabled_at_level_.insert({std::string{diag}, std::set{override}});
    }
}

void DiagEngine::disable_at_level(std::string_view diag, DiagLevel override) {
    // Only allow warns to be overridden for the warning-as-error case
    if (const auto &templ = diag_templ_for(diag); templ.level() != DiagLevel::Warning) {
        panic("cannot change diagnostic enablement level for non-warning diagnostics");
    }

    // Only allow warnings to be upgraded to err or downgraded to warn
    if (override != DiagLevel::Warning && override != DiagLevel::Error) {
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

DiagLevel DiagEngine::enabled_at(std::string_view diag) const {
    if (!enabled_at_level_.contains(diag.data())) {
        return DiagLevel::Ignored;
    }
    assert_or_panic(!enabled_at_level_.at(diag.data()).empty(),
                    fmt::format("enabled_at_level_[{}] - set was empty!", diag.data()));
    // Return the highest level present
    return *enabled_at_level_.at(diag.data()).rbegin();
}

std::uint64_t DiagEngine::in_flight_count_for_level(DiagLevel level) const {
    return std::ranges::count(in_flight_diags_, level, &InFlightDiag::level);
}

std::uint64_t DiagEngine::in_flight_count_for(std::string_view diag) const {
    auto diag_str = std::string{diag};
    if (!diag_counts_.contains(diag_str)) {
        return 0;
    }
    return diag_counts_.at(diag_str);
}

const DiagConsumer &DiagEngine::consumer() const {
    return *consumer_;
}

// ReSharper disable once CppParameterMayBeConst
DiagLevel DiagEngine::compute_level(std::string_view diag) const {
    const auto &templ = diag_templ_for(diag);

    // Only warnings can "change" levels, so short circuit on anything else
    if (templ.level() != DiagLevel::Warning) {
        return templ.level();
    }

    // Return level override if present
    if (auto diag_str = std::string{diag}; enabled_at_level_.contains(diag_str)) {
        assert_or_panic(!enabled_at_level_.at(diag_str).empty(),
                        fmt::format("enabled_at_level_[{}] - set was empty!", diag_str));
        // Return the highest level present
        return *enabled_at_level_.at(diag_str).rbegin();
    }

    // Default to the default level from the template
    return templ.level();
}

[[nodiscard]] bool DiagEngine::is_enabled(std::string_view diag) const {

    /*
     * If this diagnostic is a note, remark, error, or fatal by default, it is
     * always enabled.
     */
    // TODO : should we have note always enabled? Or should we have the generic
    // error and fatal diagnostics contain a blank note partner? The downside
    // to that approach is we're locked in to having only a single note partner
    if (const auto &templ = diag_templ_for(diag);
        templ.level() == DiagLevel::Note || templ.level() == DiagLevel::Remark || templ.level() == DiagLevel::Error ||
        templ.level() == DiagLevel::Fatal) {
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

std::string DiagEngine::construct_msg_str(const DiagLevel in_flight_level, const DiagTempl &templ,
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
