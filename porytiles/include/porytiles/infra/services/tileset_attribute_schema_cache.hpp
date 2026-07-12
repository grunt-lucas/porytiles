#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "gsl/pointers"

#include "porytiles/domain/services/enum_map_provider.hpp"
#include "porytiles/domain/services/metatile_attribute_schema_loader.hpp"
#include "porytiles/infra/services/tileset_attribute_schema_resolver.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief Resolves and caches the attribute schema and field providers for every tileset a command touches.
///
/// @details
/// A command can work with more than one tileset: compiling a secondary also reads its paired primary's artifacts, and
/// the two can resolve different schemas (per-tileset metatile_attribute_field_overrides, or use_frlg_alternate_masks
/// selecting different layouts). Every artifact must be decoded with its owning tileset's schema, so consumers that
/// read another tileset's artifacts ask this cache by tileset name instead of holding a single resolved schema.
///
/// Each tileset resolves at most once. The entry (the resolved schema plus the ProviderMap built for it) is cached with
/// a stable address for the cache's lifetime, so the resolver's diagnostics are emitted once per tileset and consumers
/// may hold the returned pointer.
class TilesetAttributeSchemaCache {
  public:
    /// @brief One tileset's resolved schema together with the providers built for its provider-backed fields.
    ///
    /// @details
    /// The providers uphold the ProviderMap membership contract for the entry's schema: the map contains exactly the
    /// schema's has_provider() fields, keyed by field name.
    struct Entry {
        ResolvedTilesetAttributeSchema resolved;
        ProviderMap providers;
    };

    /// @brief Constructs a cache that resolves entries through the given resolver.
    ///
    /// @param project_root The decomp project root the provider header paths are resolved against
    /// @param resolver The per-tileset schema resolver
    /// @param format The text formatter for styled output
    /// @param diag The user diagnostics for error reporting
    TilesetAttributeSchemaCache(
        std::filesystem::path project_root,
        gsl::not_null<const TilesetAttributeSchemaResolver *> resolver,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag)
        : project_root_{std::move(project_root)}, resolver_{resolver}, format_{format}, diag_{diag}
    {
    }

    /// @brief Returns the entry for the named tileset, resolving and caching it on first request.
    ///
    /// @details
    /// A failed resolution is not cached: a later call re-resolves, which matters only for callers that swallow the
    /// error and retry (none today; command startup treats a target failure as fatal).
    ///
    /// @param tileset_name The tileset label, used as the config scope and the layouts.json tileset reference
    /// @return A pointer to the entry, stable for the cache's lifetime, or the resolver's error
    [[nodiscard]] ChainableResult<const Entry *> entry(const std::string &tileset_name) const;

  private:
    std::filesystem::path project_root_;
    const TilesetAttributeSchemaResolver *resolver_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
    mutable std::map<std::string, Entry, std::less<>> cache_;
};

} // namespace porytiles
