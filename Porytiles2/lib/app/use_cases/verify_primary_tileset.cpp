#include "porytiles2/app/use_cases/verify_primary_tileset.hpp"

#include <string>

#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

[[nodiscard]] ChainableResult<void> VerifyPrimaryTileset::verify(const std::string &tileset_name) const
{
    if (!tileset_repo_->exists(tileset_name)) {
        return ChainableResult<void>{
            FormattableError{"tileset '{}' does not exist", FormatParam{tileset_name, Style::bold}}};
    }

    // TODO: implement

    return {};
}

} // namespace porytiles2
