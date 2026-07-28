#pragma once

#include <filesystem>
#include <string>

#include "gsl/pointers"

#include "porytiles/domain/algorithms/metatile_attribute_schema_reconciler.hpp"
#include "porytiles/infra/config/infra_config.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief Resolves the invocation's metatile attribute schema: fetch config, scan, infer, reconcile.
///
/// @details
/// This is a thin infra-layer orchestrator over four pieces, in order:
///
/// 1. Fetch the five attribute config values (fields, overrides, layer mask, size, declaration size) from the
///    layered config. These are pure user inputs: the CLI, YAML, and defaults chain never derives a value, so an
///    unset override knob arrives here as nullopt meaning exactly "the user did not pin this".
/// 2. Scan the project's fieldmap sources for raw facts (MetatileAttributeScanner).
/// 3. Run the pure domain inference over the facts (infer_metatile_attribute_candidates), skipped when there is no
///    fieldmap header to scan.
/// 4. Reconcile the inference with the user inputs (reconcile_metatile_attribute_schema), which decides the width,
///    the field set, the layer mask, and the declaration width, and reports how.
///
/// Each step emits its own non-fatal diagnostics directly. The scan emits under the "metatile-attribute-inference" tag,
/// and the reconciler under "metatile-attribute-schema". Inference itself emits nothing: facts it cannot settle travel
/// as conflict records on the candidate sets, and the reconciler turns each one fatal unless an override speaks to
/// it. Emitting at the point of detection means anything decided before a later fatal still reaches the user. The
/// caller supplies the filtered sink, so the non-fatal diagnostics respect the user's diagnostic include/exclude
/// patterns.
class MetatileAttributeSchemaResolver {
  public:
    MetatileAttributeSchemaResolver(
        std::filesystem::path project_root,
        gsl::not_null<const InfraConfig *> config,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag);

    /// @brief Resolves the metatile attribute schema for the invocation.
    ///
    /// @details
    /// The result is invocation-global: the attribute layout is one-per-project, so every tileset a command touches
    /// shares the schema resolved here. The tileset name parameter is the YAML config scope key (a per-tileset
    /// config.yaml can technically answer the fieldmap keys; the values are documented as project-level).
    ///
    /// @param tileset_name The command's target tileset, used as the config scope.
    /// @return The resolved schema, or a hard error (dual-layout ambiguity, selection failure, or invalid fields).
    [[nodiscard]] ChainableResult<LoadedMetatileAttributeSchema> resolve(const std::string &tileset_name) const;

  private:
    std::filesystem::path project_root_;
    const InfraConfig *config_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles
