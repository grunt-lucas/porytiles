#pragma once

#include <set>
#include <string>

#include "gsl/pointers"

#include "porytiles2/domain/models/tileset_name.hpp"
#include "porytiles2/domain/services/tileset_name_provider.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief TODO: fill in
 */
class ProjectTilesetNameProvider : TilesetNameProvider {
  public:
    explicit ProjectTilesetNameProvider(
        std::filesystem::path project_root,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag)
        : project_root_{std::move(project_root)}, format_{format}, diag_{diag}
    {
    }

    [[nodiscard]] ChainableResult<std::set<TilesetName>> all_tileset_names() const override;

  private:
    std::filesystem::path project_root_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles2
