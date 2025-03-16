#pragma once

#include <algorithm>
#include <memory>
#include <ranges>
#include <sstream>
#include <unordered_map>

#include "../panic/panic.hpp"
#include "./diagnostics.hpp"

namespace porytiles {

/**
 * @brief diag_engine coordinates the generation and consumption of diagnostic
 * messages.
 *
 * @details
 * diag_engine manages settings for enabling, disabling, treating warnings as
 * errors, etc. It uses a diag_consumer to process the generated diagnostics
 * according to the engine client's preference.
 */
class diag_engine {
  public:
    diag_engine()
        : consumer_(std::make_unique<ignore_consumer>()), all_warnings_enabled_{false}, all_warnings_disabled_{false},
          all_warnings_as_errors_{false} {}

    explicit diag_engine(std::unique_ptr<diag_consumer> consumer)
        : consumer_(std::move(consumer)), all_warnings_enabled_{false}, all_warnings_disabled_{false},
          all_warnings_as_errors_{false} {}

    void enable(std::string_view diag);

    void disable(std::string_view diag);

    void enable_all_warnings();

    void disable_all_warnings();

    void enable_all_warnings_as_errors();

    void override_level(std::string_view diag, diag_level override);

    [[nodiscard]] std::uint64_t in_flight_count_for_level(diag_level level) const;

    [[nodiscard]] std::uint64_t count_for(std::string_view diag) const;

    template <typename... T> void report(std::string_view diag, T &&...args) {
        // If this diagnostic is not enabled, exit now
        if (!is_enabled(diag)) {
            return;
        }
        const auto &templ = diag_templ_for(diag);

        // Compute in-flight level based on user settings
        const auto in_flight_level = compute_level(diag);

        report_helper(templ, in_flight_level, std::forward<T>(args)...);

        // Increment diagnostic counts
        auto diag_str = std::string{diag};
        if (!diag_counts_.contains(diag_str)) {
            diag_counts_.insert({diag_str, 0});
        }
        ++diag_counts_[diag_str];
    }

    template <typename... T> void report_partner(std::string_view diag, std::size_t partner_index, T &&...args) {
        const auto &parent_templ = diag_templ_for(diag);

        if (partner_index >= parent_templ.partner_diags().size()) {
            panic(fmt::format("partner index {} out of bounds for diag {}", partner_index, diag));
        }

        // If this diagnostic is not enabled, exit now
        if (!is_enabled(diag)) {
            return;
        }

        const auto &partner_templ = parent_templ.partner_diags().at(partner_index);
        const auto in_flight_level = partner_templ.level();

        report_helper(partner_templ, in_flight_level, std::forward<T>(args)...);
    }

    [[nodiscard]] const diag_consumer &consumer() const;

  private:
    std::unique_ptr<diag_consumer> consumer_{};
    bool all_warnings_enabled_;
    bool all_warnings_disabled_;
    bool all_warnings_as_errors_;
    std::unordered_map<std::string, bool> explicit_enablement_;
    std::unordered_map<std::string, diag_level> level_overrides_;
    std::unordered_map<std::string, std::uint64_t> diag_counts_;
    std::vector<in_flight_diag> in_flight_diags_;

    template <typename... T> void report_helper(const diag_templ &templ, diag_level in_flight_level, T &&...args) {
        // Fill in message template
        std::vector<std::string> raw_msg;
        try {
            raw_msg = templ.build_dynamic_msg(consumer_->is_a_tty(), in_flight_level, std::forward<T>(args)...);
        } catch (const std::exception &e) {
            panic(fmt::format("{} build_message failed: {}:", templ.name(), e.what()));
        }

        // Construct the message string for the consumer
        if (raw_msg.empty()) {
            panic(fmt::format("diagnostic {} raw_msg vector was empty", templ.name()));
        }
        const std::string constructed_msg = construct_msg_str(in_flight_level, templ, raw_msg);

        // Set diagnostic in-flight and then consume it
        const auto in_flight = in_flight_diag{in_flight_level, constructed_msg, templ};
        in_flight_diags_.push_back(in_flight);
        consumer_->consume(in_flight);
    }

    void set_enablement(std::string_view diag, bool enablement);

    [[nodiscard]] diag_level compute_level(std::string_view diag) const;

    [[nodiscard]] bool is_enabled(std::string_view diag) const;

    [[nodiscard]] std::string construct_msg_str(diag_level in_flight_level, const diag_templ &templ,
                                                const std::vector<std::string> &msg) const;
};

} // namespace porytiles
