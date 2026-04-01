#include "porytiles2/app/use_cases/secondary_tileset_helpers.hpp"

#include <memory>
#include <ranges>
#include <set>
#include <string>

#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

ChainableResult<std::unique_ptr<Tileset>> resolve_partner_primary(
    const std::string &tileset_name,
    PrimaryPairingMode pairing_mode,
    const std::vector<std::string> &partners,
    const TilesetRepo *tileset_repo,
    const TilesetMetadataProvider *metadata_provider,
    const LayoutMetadataProvider *layout_metadata_provider,
    const PorytilesTilesetManager *tileset_manager,
    const UserDiagnostics *diag)
{
    std::string partner_name{};

    switch (pairing_mode) {
    case PrimaryPairingMode::off: {
        if (!partners.empty()) {
            diag->warning(
                "pairing-partners-ignored",
                "Primary pairing mode is 'off' but '{}' has non-empty pairing partners list. Partners will be ignored.",
                FormatParam{tileset_name, Style::bold});
        }
        return std::unique_ptr<Tileset>{nullptr};
    }

    case PrimaryPairingMode::manual: {
        if (partners.empty()) {
            return FormattableError{
                "Primary pairing mode is 'manual' but no partner primaries configured for '{}'.",
                FormatParam{tileset_name, Style::bold}};
        }
        partner_name = partners.at(0);
        break;
    }

    case PrimaryPairingMode::automatic: {
        if (!partners.empty()) {
            diag->warning(
                "pairing-partners-ignored",
                "Primary pairing mode is 'automatic' but '{}' has non-empty pairing partners list. Partners will be "
                "ignored.",
                FormatParam{tileset_name, Style::bold});
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
            // TODO: eventually support multiple distinct primaries
            diag->warning(
                "multiple-partner-primaries",
                "Multiple distinct partner primaries found for '{}'. Using first found: '{}'.",
                FormatParam{tileset_name, Style::bold},
                FormatParam{*found_primaries.begin(), Style::bold});
        }

        partner_name = *found_primaries.begin();
        break;
    }
    }

    if (!metadata_provider->exists(partner_name)) {
        return FormattableError{
            "Resolved partner primary '{}' for secondary '{}' does not exist.",
            FormatParam{partner_name, Style::bold},
            FormatParam{tileset_name, Style::bold}};
    }
    if (!tileset_manager->is_porytiles_managed(partner_name)) {
        return FormattableError{
            "Secondary compilation requires a Porytiles-managed primary, but '{}' is not Porytiles-managed.",
            FormatParam{partner_name, Style::bold}};
    }

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
