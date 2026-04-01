#pragma once

#include <memory>
#include <string>
#include <vector>

#include "porytiles2/app/config/primary_pairing_mode.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/services/layout_metadata_provider.hpp"
#include "porytiles2/domain/services/porytiles_tileset_manager.hpp"
#include "porytiles2/domain/services/tileset_metadata_provider.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

class TilesetRepo;

/**
 * @brief Resolves the partner primary tileset for a secondary tileset.
 *
 * @details
 * Given a secondary tileset name and pairing configuration, determines which primary tileset to pair with. Supports
 * three pairing modes:
 * - @c off: No primary pairing, returns nullptr.
 * - @c manual: Uses the first entry from the configured partners list.
 * - @c automatic: Scans project layouts to find which primary is paired with this secondary.
 *
 * After resolution, validates that the partner primary exists and is Porytiles-managed, then loads and returns it.
 *
 * @param tileset_name The name of the secondary tileset being compiled or created.
 * @param pairing_mode The configured primary pairing mode.
 * @param partners The configured list of partner primary tileset names.
 * @param tileset_repo Repository for loading tileset data.
 * @param metadata_provider Provider for checking tileset existence.
 * @param layout_metadata_provider Provider for scanning project layouts (used in automatic mode).
 * @param tileset_manager Manager for checking Porytiles ownership.
 * @param diag Diagnostics interface for warnings.
 * @return The loaded partner primary Tileset, nullptr if pairing is off, or an error.
 */
[[nodiscard]] ChainableResult<std::unique_ptr<Tileset>> resolve_partner_primary(
    const std::string &tileset_name,
    PrimaryPairingMode pairing_mode,
    const std::vector<std::string> &partners,
    const TilesetRepo *tileset_repo,
    const TilesetMetadataProvider *metadata_provider,
    const LayoutMetadataProvider *layout_metadata_provider,
    const PorytilesTilesetManager *tileset_manager,
    const UserDiagnostics *diag);

} // namespace porytiles2
