#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>

#include "gsl/pointers"

#include "porytiles/domain/config/metatile_attr_field_spec.hpp"
#include "porytiles/infra/config/config_provider.hpp"
#include "porytiles/infra/config/layer_value.hpp"
#include "porytiles/infra/config/metatile_attr_inference.hpp"
#include "porytiles/infra/config/metatiles_header_provider.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief A ConfigProvider that synthesizes a metatile attribute field schema from a decomp project's own headers.
///
/// @details
/// When a user has not written an explicit @c metatile_attr_fields config, this provider reads the project's
/// @c include/global.fieldmap.h, @c src/fieldmap.c, and @c include/constants/metatile_behaviors.h and infers the field
/// schema the user could have written by hand. It sits in the provider chain below the explicit YAML provider so a
/// hand-written schema always wins.
///
/// The provider performs the file I/O and hands the gathered facts to the pure inference in
/// metatile_attr_inference.hpp. Inference warnings and conflicts are routed to the user diagnostics. The result is
/// computed once per config scope and cached under a @c type:scope key (mirroring LazyLayeredConfig), so inference
/// warnings are emitted a single time per scope. Every command uses a single scope per run, so there is no visible
/// duplication in practice.
///
/// Besides the field schema, the same cached inference run also answers @c metatile_layer_type_mask and
/// @c metatile_layer_type_mask_frlg: the layer-type bit mask the base game declares (primary and FRLG-alternate). Each
/// is answered only when the base game declared a mask for that layout; otherwise it defers so the size-based default
/// fallback applies. The user's explicit config still wins over this inference via the provider chain order.
///
/// Outcomes mirror the inference outcomes:
/// - valid: an inferred field set is returned.
/// - invalid: the project declares fields whose masks could not be determined; a fatal, actionable error is returned.
/// - not_provided: nothing attribute-related was found, so the next provider is consulted.
///
/// Unlike the other providers in this directory, this class is handwritten rather than generated from
/// config_schema.yaml. The generated providers map every config value to a uniform source (a YAML path, a header
/// define, a CLI option); this one answers just three values through a bespoke inference pipeline, so there is
/// nothing schema-shaped to generate. It only overrides the methods it answers and defers the rest to the
/// ConfigProvider base class defaults.
class MetatileAttributeConfigProvider final : public ConfigProvider {
  public:
    /// @brief Constructs a MetatileAttributeConfigProvider.
    ///
    /// @param project_root The root directory of the decomp project
    /// @param format The formatter used for diagnostic text
    /// @param diagnostics The sink for inference warnings
    MetatileAttributeConfigProvider(
        std::filesystem::path project_root,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diagnostics);

    [[nodiscard]] std::string name() const override;

    [[nodiscard]] LayerValue<MetatileAttrFieldSpecs>
    metatile_attr_fields(ConfigScopeType type, const std::string &scope) const override;

    [[nodiscard]] LayerValue<std::optional<std::uint32_t>>
    metatile_layer_type_mask(ConfigScopeType type, const std::string &scope) const override;

    [[nodiscard]] LayerValue<std::optional<std::uint32_t>>
    metatile_layer_type_mask_frlg(ConfigScopeType type, const std::string &scope) const override;

  private:
    /// @brief The cached outcome of one inference run over a config scope.
    ///
    /// @details
    /// compute() runs the file I/O and inference exactly once per scope. The three config values this provider answers
    /// are all derived from the same cached run, so inference warnings are emitted a single time. @c provided is false
    /// when there is no fieldmap header to infer from (or it could not be scanned), in which case every value defers.
    struct CachedInference {
        bool provided{false};
        MetatileAttrInferenceResult result;
        std::string source; ///< provenance source key (the fieldmap header path)
    };

    [[nodiscard]] const CachedInference &inference_for(ConfigScopeType type, const std::string &scope) const;
    [[nodiscard]] CachedInference compute(ConfigScopeType type, const std::string &scope) const;

    std::filesystem::path project_root_;
    const TextFormatter *format_;
    const UserDiagnostics *diagnostics_;
    MetatilesHeaderProvider metatiles_provider_;
    mutable std::map<std::string, CachedInference> cached_results_;
};

} // namespace porytiles
