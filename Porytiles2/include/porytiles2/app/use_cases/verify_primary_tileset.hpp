#pragma once

#include <string>

#include "gsl/pointers"

#include "porytiles2/app/config/app_config.hpp"
#include "porytiles2/domain/config/domain_config.hpp"
#include "porytiles2/domain/repos/tileset_repo.hpp"
#include "porytiles2/domain/services/primary_tileset_compiler.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles2 {

/**
 * @brief Use case for verifying a primary tileset.
 */
class VerifyPrimaryTileset {
  public:
    /**
     * @brief Constructs a VerifyPrimaryTileset use case with the given repositories and services.
     *
     * @param tileset_repo A pointer to the TilesetRepo for this use case.
     * @param compiler A pointer to the PrimaryTilesetCompiler for this use case.
     * @param domain_config A pointer to the DomainConfig for this use case
     * @param app_config A pointer to the AppConfig for this use case
     * @param format A pointer to a TextFormatter for this use case
     * @param diag A pointer to the UserDiagnostics for this use case
     */
    VerifyPrimaryTileset(
        gsl::not_null<const TilesetRepo *> tileset_repo,
        gsl::not_null<const PrimaryTilesetCompiler *> compiler,
        gsl::not_null<const DomainConfig *> domain_config,
        gsl::not_null<const AppConfig *> app_config,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag)
        : tileset_repo_{tileset_repo}, compiler_{compiler}, domain_config_{domain_config}, app_config_{app_config},
          format_{format}, diag_{diag}
    {
    }

    [[nodiscard]] ChainableResult<void> verify(const std::string &tileset_name) const;

  private:
    const TilesetRepo *tileset_repo_;
    const PrimaryTilesetCompiler *compiler_;
    const DomainConfig *domain_config_;
    const AppConfig *app_config_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles2
