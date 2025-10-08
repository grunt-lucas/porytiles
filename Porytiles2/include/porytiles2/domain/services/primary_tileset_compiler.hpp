#pragma once

#include <memory>

#include "gsl/pointers"

#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

/**
 * @brief Service that compiles a primary Tileset.
 */
class PrimaryTilesetCompiler {
  public:
    explicit PrimaryTilesetCompiler(
        gsl::not_null<TextFormatter *> text_formatter, gsl::not_null<UserDiagnostics *> diag)
        : text_formatter_{text_formatter}, diag_{diag}
    {
    }

    [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>> compile(const Tileset &tileset);

    [[nodiscard]] ChainableResult<std::unique_ptr<Tileset>> compile_incremental(const Tileset &tileset);

  private:
    TextFormatter *text_formatter_;
    UserDiagnostics *diag_;
};

} // namespace porytiles2
