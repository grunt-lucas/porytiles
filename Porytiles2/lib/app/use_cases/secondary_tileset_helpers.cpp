#include "porytiles2/app/use_cases/secondary_tileset_helpers.hpp"

#include <memory>
#include <ranges>
#include <set>
#include <string>

#include "porytiles2/domain/algorithms/diagnostic_stencils.hpp"
#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<std::unique_ptr<Tileset>> resolve_partner_primary(
    const std::string &tileset_name,
    const ConfigValue<PrimaryPairingMode> &pairing_mode,
    const ConfigValue<std::vector<std::string>> &partners,
    const TilesetRepo *tileset_repo,
    const TilesetMetadataProvider *metadata_provider,
    const LayoutMetadataProvider *layout_metadata_provider,
    const PorytilesTilesetManager *tileset_manager,
    const UserDiagnostics *diag)
{
    std::string partner_name{};

    switch (pairing_mode.value()) {
    case PrimaryPairingMode::off: {
        if (!partners.value().empty()) {
            diag->warning(
                "pairing-partners-ignored",
                std::vector<std::string>{
                    diag->formatter().format(
                        "Primary pairing mode is 'off' but '{}' has non-empty pairing partners list.",
                        FormatParam{tileset_name, Style::bold}),
                    "Partners will be ignored."});
        }
        return std::unique_ptr<Tileset>{nullptr};
    }

    case PrimaryPairingMode::manual: {
        if (partners.value().empty()) {
            std::vector<std::string> err_msg{};
            err_msg.emplace_back(diag->formatter().format(
                "Primary pairing mode is 'manual' but no partner primaries configured for '{}'.",
                FormatParam{tileset_name, Style::bold}));
            err_msg.append_range(format_config_note_with_separator(diag->formatter(), pairing_mode));
            err_msg.append_range(format_config_note_with_separator(diag->formatter(), partners));
            return FormattableError{err_msg};
        }
        partner_name = partners.value().at(0);
        break;
    }

    case PrimaryPairingMode::automatic: {
        if (!partners.value().empty()) {
            constexpr auto warn_tag = "pairing-partners-ignored";
            diag->warning(
                warn_tag,
                std::vector<std::string>{
                    diag->formatter().format(
                        "Primary pairing mode is 'automatic' but '{}' has non-empty pairing partners list.",
                        FormatParam{tileset_name, Style::bold}),
                    "Partners will be ignored."});
            std::vector<std::string> note_lines{};
            note_lines.append_range(format_config_note(diag->formatter(), pairing_mode));
            note_lines.append_range(format_config_note_with_separator(diag->formatter(), partners));
            diag->warning_note(warn_tag, note_lines);
        }

        PT_TRY_ASSIGN_CHAIN_ERR(
            layout_names,
            layout_metadata_provider->layout_names(),
            std::unique_ptr<Tileset>,
            "Failed to enumerate layouts for automatic primary pairing.");

        std::set<std::string> found_primaries{};
        for (const auto &layout_name : layout_names) {
            auto secondary_result = layout_metadata_provider->secondary_tileset(layout_name);
            if (!secondary_result.has_value()) {
                continue;
            }
            if (secondary_result.value() == tileset_name) {
                auto primary_result = layout_metadata_provider->primary_tileset(layout_name);
                if (primary_result.has_value()) {
                    found_primaries.insert(primary_result.value());
                }
            }
        }

        if (found_primaries.empty()) {
            return FormattableError{
                "Automatic primary pairing found no layouts using secondary tileset '{}'.",
                FormatParam{tileset_name, Style::bold}};
        }

        if (found_primaries.size() > 1) {
            std::vector<std::string> warn_lines{};
            warn_lines.push_back(diag->formatter().format(
                "Multiple distinct partner primaries found for '{}' via layout data.",
                FormatParam{tileset_name, Style::bold}));
            warn_lines.push_back(diag->formatter().format(
                "Using first found: '{}'.", FormatParam{*found_primaries.begin(), Style::bold}));
            warn_lines.emplace_back("Other partners:");
            for (const auto &name : found_primaries | std::views::drop(1)) {
                warn_lines.push_back(diag->formatter().format("  '{}'", FormatParam{name, Style::bold}));
            }
            diag->warning("multiple-partner-primaries", warn_lines);
        }

        partner_name = *found_primaries.begin();
        break;
    }
    }

    if (!metadata_provider->exists(partner_name)) {
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(diag->formatter().format(
            "Resolved partner primary '{}' for secondary '{}' does not exist.",
            FormatParam{partner_name, Style::bold},
            FormatParam{tileset_name, Style::bold}));
        err_msg.emplace_back("Create it first or pair with a different tileset.");
        err_msg.append_range(format_config_note_with_separator(diag->formatter(), pairing_mode));
        err_msg.append_range(format_config_note_with_separator(diag->formatter(), partners));
        return FormattableError{err_msg};
    }
    if (!tileset_manager->is_porytiles_managed(partner_name)) {
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(diag->formatter().format(
            "Secondary compilation requires a Porytiles-managed primary, but '{}' is not Porytiles-managed.",
            FormatParam{partner_name, Style::bold}));
        err_msg.emplace_back("Import it first or pair with a different tileset.");
        err_msg.append_range(format_config_note_with_separator(diag->formatter(), pairing_mode));
        err_msg.append_range(format_config_note_with_separator(diag->formatter(), partners));
        return FormattableError{err_msg};
    }

    constexpr auto tag = "resolve-partner-primary";
    diag->remark(
        tag,
        "Resolved partner primary '{}' for secondary '{}'.",
        FormatParam{partner_name, Style::bold},
        FormatParam{tileset_name, Style::bold});
    std::vector<std::string> note_lines{};
    note_lines.append_range(format_config_note(diag->formatter(), pairing_mode));
    note_lines.append_range(format_config_note_with_separator(diag->formatter(), partners));
    diag->remark_note(tag, note_lines);

    PT_TRY_ASSIGN_CHAIN_ERR(
        primary_tileset,
        tileset_repo->load(partner_name),
        std::unique_ptr<Tileset>,
        "Failed to load partner primary '{}' for secondary '{}'.",
        FormatParam{partner_name, Style::bold},
        FormatParam{tileset_name, Style::bold});

    return primary_tileset;
}

} // namespace porytiles2
