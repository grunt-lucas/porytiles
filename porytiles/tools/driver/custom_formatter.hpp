#pragma once

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "CLI/CLI.hpp"

namespace porytiles {

/// @brief Custom CLI11 formatter for cleaner help output.
///
/// @details
/// This formatter customizes CLI11's default help output by:
/// - Hiding verbose type labels (REQUIRED, TEXT, INT, etc.)
/// - Using clean option group headers without "[Option Group: ...]" prefix
/// - Stripping verbose transformer descriptions from enum type names
/// - Adjusting column width for better alignment
class PorytilesFormatter : public CLI::Formatter {
  public:
    PorytilesFormatter() : CLI::Formatter()
    {
        // Hide verbose type labels
        label("REQUIRED", "");
        label("TEXT", "");
        label("INT", "");
        label("UINT", "");
        label("FLOAT", "");

        // Adjust column width for better alignment
        column_width(40);
    }

    /// @brief Override to customize option group headers.
    ///
    /// @details
    /// Removes the "[Option Group: ...]" prefix and uses a clean "GroupName:" format.
    std::string make_group(std::string group, bool is_positional, std::vector<const CLI::Option *> opts) const override
    {
        if (opts.empty()) {
            return "";
        }

        std::stringstream out;

        // Use clean header format like standard CLIs
        out << "\n" << group << ":\n";

        for (const auto *opt : opts) {
            out << make_option(opt, is_positional);
        }

        return out.str();
    }

    /// @brief Override to handle option groups (inline subcommands) with clean headers.
    ///
    /// @details
    /// CLI11 renders option groups as expanded subcommands. This override provides
    /// a cleaner format that doesn't use "[Option Group: ...]" style headers.
    /// It also filters out the help flag to avoid duplication and outputs options
    /// directly without intermediate group headers.
    std::string make_expanded(const CLI::App *sub, CLI::AppFormatMode mode) const override
    {
        std::stringstream out;

        // For option groups (empty name, non-empty group), use the group name as header
        // instead of get_display_name() which produces "[Option Group: ...]"
        bool is_option_group = sub->get_name().empty() && !sub->get_group().empty();
        std::string header;
        if (is_option_group) {
            header = sub->get_group();
        }
        else {
            header = sub->get_display_name(true);
        }

        out << "\n" << header << ":\n";

        // Include description if present
        std::string desc = sub->get_description();
        if (!desc.empty()) {
            out << "  " << desc << "\n";
        }

        // Include positionals
        out << make_positionals(sub);

        if (is_option_group) {
            // For option groups, directly output options without group headers
            // This avoids duplicate "OPTIONS:" headers and filters out help flags
            std::vector<const CLI::Option *> opts = sub->get_options([sub](const CLI::Option *opt) {
                return opt->nonpositional() && sub->get_help_ptr() != opt && sub->get_help_all_ptr() != opt;
            });

            for (const CLI::Option *opt : opts) {
                out << make_option(opt, false);
            }
        }
        else {
            // For regular subcommands, use standard group formatting
            out << make_groups(sub, mode);
        }

        // Include subcommands
        out << make_subcommands(sub, mode);

        return out.str();
    }

    /// @brief Override to strip verbose transformer descriptions from type names.
    ///
    /// @details
    /// CLI11's CheckedTransformer appends verbose descriptions like
    /// ":value in {locked->locked,...} OR {locked,...}" after the type name.
    /// This override strips everything after the first colon to show just the
    /// clean type name like "{locked|patch|optimize}".
    std::string make_option_opts(const CLI::Option *opt) const override
    {
        std::stringstream out;

        if (!opt->get_option_text().empty()) {
            out << " " << opt->get_option_text();
        }
        else {
            if (opt->get_type_size() != 0) {
                if (!opt->get_type_name().empty()) {
                    std::string type_name = opt->get_type_name();

                    // Strip verbose transformer descriptions (everything after ":")
                    // This handles CheckedTransformer's "TYPE:value in {...}" format
                    std::size_t colon_pos = type_name.find(':');
                    if (colon_pos != std::string::npos) {
                        type_name = type_name.substr(0, colon_pos);
                    }

                    // Apply label transformation (may convert TEXT/INT/etc to empty)
                    std::string label_val = get_label(type_name);
                    if (!label_val.empty()) {
                        out << " " << label_val;
                    }
                }
                if (!opt->get_default_str().empty()) {
                    out << " [" << opt->get_default_str() << "]";
                }
                if (opt->get_expected_max() == CLI::detail::expected_max_vector_size) {
                    out << " ...";
                }
                else if (opt->get_expected_min() > 1) {
                    out << " x " << opt->get_expected();
                }

                if (opt->get_required()) {
                    std::string req_label = get_label("REQUIRED");
                    if (!req_label.empty()) {
                        out << " " << req_label;
                    }
                }
            }
            if (!opt->get_envname().empty()) {
                out << " (" << get_label("Env") << ":" << opt->get_envname() << ")";
            }
            if (!opt->get_needs().empty()) {
                out << " " << get_label("Needs") << ":";
                for (const CLI::Option *op : opt->get_needs()) {
                    out << " " << op->get_name();
                }
            }
            if (!opt->get_excludes().empty()) {
                out << " " << get_label("Excludes") << ":";
                for (const CLI::Option *op : opt->get_excludes()) {
                    out << " " << op->get_name();
                }
            }
        }
        return out.str();
    }
};

} // namespace porytiles
