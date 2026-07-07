#pragma once

#include <string>

#include "gsl/pointers"

#include "porytiles/domain/services/metatile_attr_schema_loader.hpp"
#include "porytiles/infra/config/lazy_layered_config.hpp"
#include "porytiles/infra/services/project_layout_metadata_provider.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/**
 * @brief Resolves the metatile attribute schema for a single tileset, choosing the primary or FRLG layout.
 *
 * @details
 * This is the infra-layer adapter that gathers the inputs to the pure domain resolve_tileset_attr_schema: it fetches
 * the field specs, overrides, attribute size, and the FRLG mask mode from config at tileset scope, decides which layout
 * applies, and reports its reasoning through the user diagnostics.
 *
 * Layout selection follows use_frlg_alternate_masks: @c always and @c never force the choice, while @c automatic
 * cross-references data/layouts/layouts.json via the layout metadata provider. In automatic mode a tileset referenced
 * only by frlg layouts selects the FRLG masks (with a remark citing the evidence), emerald-only or unreferenced
 * tilesets select the primary masks, a tileset used by both is a hard error naming the escape hatch, and a malformed
 * layouts.json downgrades to a warning plus primary fallback.
 *
 * The attribute byte width is treated as explicit only when the config value came from the CLI or a YAML file; a value
 * synthesized by the metatiles-header or default provider is not explicit and may be widened to cover the FRLG masks.
 */
class TilesetAttrSchemaResolver {
  public:
    TilesetAttrSchemaResolver(
        gsl::not_null<const LazyLayeredConfig *> config,
        gsl::not_null<const ProjectLayoutMetadataProvider *> layout_metadata,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag);

    /**
     * @brief Resolves the attribute schema for the named tileset.
     *
     * @param tileset_name The tileset label, used as both config scope and the layouts.json tileset reference.
     * @return The resolved schema, or a hard error (mixed layout usage, invalid layout_version, or an invalid layout).
     */
    [[nodiscard]] ChainableResult<ResolvedTilesetAttrSchema> resolve(const std::string &tileset_name) const;

  private:
    const LazyLayeredConfig *config_;
    const ProjectLayoutMetadataProvider *layout_metadata_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles
