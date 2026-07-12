#include "porytiles/infra/services/tileset_attribute_schema_cache.hpp"

#include <string>
#include <utility>

#include "porytiles/infra/services/header_enum_map_provider.hpp"

namespace porytiles {

ChainableResult<const TilesetAttributeSchemaCache::Entry *>
TilesetAttributeSchemaCache::entry(const std::string &tileset_name) const
{
    if (const auto it = cache_.find(tileset_name); it != cache_.end()) {
        return &it->second;
    }

    PT_TRY_ASSIGN_PASS_ERR(resolved, resolver_->resolve(tileset_name), const Entry *);
    ProviderMap providers = build_provider_map(project_root_, resolved.schema, format_, diag_);
    const auto emplaced = cache_.emplace(tileset_name, Entry{std::move(resolved), std::move(providers)});
    return &emplaced.first->second;
}

} // namespace porytiles
