#pragma once

/**
 * @file diagnostic_engine.hpp Implementation for the diagnostic engine.
 * @copyright Copyright 2025 grunt-lucas. All rights reserved. This project is licensed under the MIT License.
 */

#include <algorithm>
#include <memory>
#include <ranges>
#include <set>
#include <sstream>
#include <unordered_map>

#include "../panic/panic.hpp"
#include "./diagnostics.hpp"

namespace porytiles {

/**
 * @brief Coordinates the generation and consumption of diagnostic messages
 *
 * @details
 * DiagEngine manages settings for enabling, disabling, treating warnings as errors, etc. It uses a DiagConsumer to
 * process the generated diagnostics according to the engine client's preferences.
 */
class DiagEngine {
  public:
    DiagEngine() : consumer_(std::make_unique<IgnoreConsumer>()), all_warnings_disabled_{false} {}

    explicit DiagEngine(std::unique_ptr<DiagConsumer> consumer)
        : consumer_(std::move(consumer)), all_warnings_disabled_{false} {}

    void EnableAllWarnings();

    void DisableAllWarnings();

    void UpgradeEnabledWarningsToErr();

    void EnableAtLevel(std::string_view diag, DiagLevel override);

    void DisableAtLevel(std::string_view diag, DiagLevel override);

    [[nodiscard]] DiagLevel EnabledAt(std::string_view diag) const;

    [[nodiscard]] std::uint64_t InFlightCountForLevel(DiagLevel level) const;

    [[nodiscard]] std::uint64_t InFlightCountFor(std::string_view diag) const;

    // ReSharper disable once CppParameterMayBeConst
    template <typename... T>
    void Report(std::string_view diag, T &&...args) {
        // If this diagnostic is not enabled, exit now
        if (!IsEnabled(diag)) {
            return;
        }
        const auto &templ = DiagFor(diag);

        // Compute in-flight level based on user settings
        const auto in_flight_level = ComputeLevel(diag);

        ReportHelper(templ, in_flight_level, std::forward<T>(args)...);

        // Increment diagnostic counts
        auto diag_str = std::string{diag};
        if (!diag_counts_.contains(diag_str)) {
            diag_counts_.insert({diag_str, 0});
        }
        diag_counts_[diag_str] += 1;
    }

    template <typename... T>
    void ReportPartner(std::string_view diag, std::size_t partner_index, T &&...args) {
        const auto &parent_templ = DiagFor(diag);

        if (partner_index >= parent_templ.partner_diags().size()) {
            Panic(fmt::format("partner index {} out of bounds for diag {}", partner_index, diag));
        }

        // If this diagnostic is not enabled, exit now
        if (!IsEnabled(diag)) {
            return;
        }

        const auto &partner_templ = parent_templ.partner_diags().at(partner_index);
        const auto in_flight_level = partner_templ.level();

        ReportHelper(partner_templ, in_flight_level, std::forward<T>(args)...);
    }

    template <typename T>
    auto Style(const T &t, fmt::text_style ts) const {
        return fmt::styled(t, consumer_->IsATty() ? ts : fmt::text_style{});
    }

    template <typename T>
    auto Bold(const T &t) const {
        return Style(t, fmt::emphasis::bold);
    }

    [[nodiscard]] const DiagConsumer &consumer() const;

  private:
    std::unique_ptr<DiagConsumer> consumer_;
    bool all_warnings_disabled_;
    std::unordered_map<std::string, std::set<DiagLevel>> enabled_at_level_;
    std::unordered_map<std::string, std::uint64_t> diag_counts_;
    std::vector<InFlightDiag> in_flight_diags_;

    template <typename... T>
    void ReportHelper(const DiagTempl &templ, DiagLevel in_flight_level, T &&...args) {
        // Fill in message template
        std::vector<std::string> raw_msg;
        try {
            raw_msg = templ.BuildDynamicMsg(*this, in_flight_level, std::forward<T>(args)...);
        } catch (const std::exception &e) {
            Panic(fmt::format("{} build_message failed: {}:", templ.name(), e.what()));
        }

        // Construct the message string for the consumer
        if (raw_msg.empty()) {
            Panic(fmt::format("diagnostic {} raw_msg vector was empty", templ.name()));
        }
        const std::string constructed_msg = ConstructMsgStr(in_flight_level, templ, raw_msg);

        // Set diagnostic in-flight and then consume it
        const auto in_flight = InFlightDiag{in_flight_level, constructed_msg, templ};
        in_flight_diags_.push_back(in_flight);
        consumer_->Consume(in_flight);
    }

    [[nodiscard]] DiagLevel ComputeLevel(std::string_view diag) const;

    [[nodiscard]] bool IsEnabled(std::string_view diag) const;

    [[nodiscard]] std::string ConstructMsgStr(DiagLevel in_flight_level, const DiagTempl &templ,
                                              const std::vector<std::string> &msg) const;
};

} // namespace porytiles
