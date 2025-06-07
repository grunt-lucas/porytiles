#include "diagnostics/diagnostic_engine.hpp"

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

void DiagEngine::EnableAllWarnings() {
    for (const auto &diag : AllDiagNames()) {
        // Only apply enablement to diagnostics that are default-warnings
        if (const auto &templ = DiagFor(diag); templ.level() == DiagLevel::Warning) {
            EnableAtLevel(diag, DiagLevel::Warning);
        }
    }
}

void DiagEngine::DisableAllWarnings() {
    all_warnings_disabled_ = true;
}

void DiagEngine::UpgradeEnabledWarningsToErr() {
    for (const auto &diag : AllDiagNames()) {
        // Only apply enablement to diagnostics that are default-warnings
        if (const auto &templ = DiagFor(diag); templ.level() == DiagLevel::Warning) {
            if (enabled_at_level_.contains(diag)) {
                auto &set = enabled_at_level_.at(diag);
                set.insert(DiagLevel::Error);
            }
        }
    }
}

void DiagEngine::EnableAtLevel(std::string_view diag, DiagLevel override) {
    // Only allow warns to be overridden for the warning-as-error case
    if (const auto &templ = DiagFor(diag); templ.level() != DiagLevel::Warning) {
        Panic("cannot change diagnostic enablement level for non-warning diagnostics");
    }

    // Only allow warnings to be upgraded to err or downgraded to warn
    if (override != DiagLevel::Warning && override != DiagLevel::Error) {
        Panic(fmt::format("cannot override diagnostic '{}' level to {}", diag, LevelToStr(override)));
    }

    if (enabled_at_level_.contains(diag.data())) {
        auto &set = enabled_at_level_.at(diag.data());
        set.insert(override);
    } else {
        enabled_at_level_.insert({std::string{diag}, std::set{override}});
    }
}

void DiagEngine::DisableAtLevel(std::string_view diag, DiagLevel override) {
    // Only allow warns to be overridden for the warning-as-error case
    if (const auto &templ = DiagFor(diag); templ.level() != DiagLevel::Warning) {
        Panic("cannot change diagnostic enablement level for non-warning diagnostics");
    }

    // Only allow warnings to be upgraded to err or downgraded to warn
    if (override != DiagLevel::Warning && override != DiagLevel::Error) {
        Panic(fmt::format("cannot override diagnostic '{}' level to {}", diag, LevelToStr(override)));
    }

    if (enabled_at_level_.contains(diag.data())) {
        auto &set = enabled_at_level_.at(diag.data());
        set.erase(override);
        if (set.empty()) {
            enabled_at_level_.erase(diag.data());
        }
    }
}

DiagLevel DiagEngine::EnabledAt(std::string_view diag) const {
    if (!enabled_at_level_.contains(diag.data())) {
        return DiagLevel::Ignored;
    }
    AssertOrPanic(!enabled_at_level_.at(diag.data()).empty(),
                  fmt::format("enabled_at_level_[{}] - set was empty!", diag.data()));
    // Return the highest level present
    return *enabled_at_level_.at(diag.data()).rbegin();
}

std::uint64_t DiagEngine::InFlightCountForLevel(DiagLevel level) const {
    return std::ranges::count(in_flight_diags_, level, &InFlightDiag::level);
}

std::uint64_t DiagEngine::InFlightCountFor(std::string_view diag) const {
    const auto diag_str = std::string{diag};
    if (!diag_counts_.contains(diag_str)) {
        return 0;
    }
    return diag_counts_.at(diag_str);
}

const DiagConsumer &DiagEngine::consumer() const {
    return *consumer_;
}

// ReSharper disable once CppParameterMayBeConst
DiagLevel DiagEngine::ComputeLevel(std::string_view diag) const {
    const auto &templ = DiagFor(diag);

    // Only warnings can "change" levels, so short circuit on anything else
    if (templ.level() != DiagLevel::Warning) {
        return templ.level();
    }

    // Return level override if present
    if (auto diag_str = std::string{diag}; enabled_at_level_.contains(diag_str)) {
        AssertOrPanic(!enabled_at_level_.at(diag_str).empty(),
                      fmt::format("enabled_at_level_[{}] - set was empty!", diag_str));
        // Return the highest level present
        return *enabled_at_level_.at(diag_str).rbegin();
    }

    // Default to the default level from the template
    return templ.level();
}

[[nodiscard]] bool DiagEngine::IsEnabled(std::string_view diag) const {

    // If this diagnostic is a note, remark, error, or fatal by default, it is
    // always enabled.
    //
    // TODO : should we have note always enabled? Or should we have the generic
    // error and fatal diagnostics contain a blank note partner? The downside
    // to that approach is we're locked in to having only a single note partner
    if (const auto &templ = DiagFor(diag); templ.level() == DiagLevel::Note || templ.level() == DiagLevel::Remark ||
                                           templ.level() == DiagLevel::Error || templ.level() == DiagLevel::Fatal) {
        return true;
    }

    // The highest precedence is global warning disable. If this is specified,
    // all warnings (including warnings which have been upgraded to errors)
    // will be disabled. Any other override setting will be ignored.
    if (all_warnings_disabled_) {
        return false;
    }

    // The next highest precedence is an explicitly enabled or disabled
    // diagnostic. If the diagnostic is not present in this map, that means
    // it was not set by the user, so it shouldn't be enabled.
    if (const auto diag_str = std::string{diag}; enabled_at_level_.contains(diag_str)) {
        return true;
    }

    // Lowest precedence, if nothing else passed, then diagnostic is disabled
    return false;
}

std::string DiagEngine::ConstructMsgStr(const DiagLevel in_flight_level, const DiagTempl &templ,
                                        const std::vector<std::string> &msg) const {
    std::stringstream ss{};

    auto level_prefix = fmt::format("{}: ", LevelToStr(in_flight_level));
    const auto style = fmt::emphasis::bold | fg(ColorForLevel(in_flight_level));

    // If consumer is a tty, style the prefix with the appropriate color.
    if (consumer_->IsATty()) {
        level_prefix = fmt::format("{}", styled(level_prefix, style));
    }

    // Dump the level prefix followed by the first line of the message.
    ss << level_prefix << msg.at(0);

    // Warnings and warnings-as-errors show "[-Wname-of-warning]" at the end
    // of the first line, so the user can easily identify the source of the
    // diagnostic. Handle that formatting here, styling if the consumer is
    // a tty.
    if (const auto flag = construct_flag(in_flight_level, templ); flag.has_value()) {
        ss << " [";
        const auto styled_flag = fmt::format("{}", styled(flag.value(), style));
        consumer_->IsATty() ? ss << styled_flag : ss << flag.value();
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
