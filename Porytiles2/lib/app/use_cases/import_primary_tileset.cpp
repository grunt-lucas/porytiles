#include "porytiles2/app/use_cases/import_primary_tileset.hpp"

#include <memory>
#include <string>

#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/xcut/config/unwrap_config.hpp"

namespace porytiles2 {

ChainableResult<void> ImportPrimaryTileset::import(const std::string &tileset_name) const
{
    if (!tileset_metadata_provider_->exists(tileset_name)) {
        return FormattableError{"tileset '{}' does not exist", FormatParam{tileset_name, Style::bold}};
    }

    if (porytiles_tileset_manager_->is_porytiles_managed(tileset_name)) {
        return FormattableError{"tileset '{}' is already Porytiles-managed", FormatParam{tileset_name, Style::bold}};
    }

    return {};
}

} // namespace porytiles2
