#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "gsl/pointers"

#include "porytiles/domain/config/metatile_attr_field_spec.hpp"
#include "porytiles/infra/config/config_provider.hpp"
#include "porytiles/infra/config/layer_value.hpp"
#include "porytiles/infra/config/metatiles_header_provider.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/**
 * @brief A ConfigProvider that synthesizes a metatile attribute field schema from a decomp project's own headers.
 *
 * @details
 * When a user has not written an explicit @c metatile_attr_fields config, this provider reads the project's
 * @c include/global.fieldmap.h, @c src/fieldmap.c, and @c include/constants/metatile_behaviors.h and infers the field
 * schema the user could have written by hand. It sits in the provider chain below the explicit YAML provider so a
 * hand-written schema always wins.
 *
 * The provider performs the file I/O and hands the gathered facts to the pure inference in metatile_attr_inference.hpp.
 * Inference warnings and conflicts are routed to the user diagnostics. The result is computed once and cached; the
 * warnings are therefore emitted a single time.
 *
 * Outcomes mirror the inference outcomes:
 * - valid: an inferred field set is returned.
 * - invalid: the project declares fields whose masks could not be determined; a fatal, actionable error is returned.
 * - not_provided: nothing attribute-related was found, so the next provider is consulted.
 */
class MetatileAttributeConfigProvider final : public ConfigProvider {
  public:
    /**
     * @brief Constructs a MetatileAttributeConfigProvider.
     *
     * @param project_root The root directory of the decomp project
     * @param format The formatter used for diagnostic text
     * @param diagnostics The sink for inference warnings
     */
    MetatileAttributeConfigProvider(
        std::filesystem::path project_root,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diagnostics);

    [[nodiscard]] std::string name() const override;

    [[nodiscard]] LayerValue<MetatileAttrFieldSpecs>
    metatile_attr_fields(ConfigScopeType type, const std::string &scope) const override;

  private:
    [[nodiscard]] LayerValue<MetatileAttrFieldSpecs> compute(ConfigScopeType type, const std::string &scope) const;

    std::filesystem::path project_root_;
    const TextFormatter *format_;
    const UserDiagnostics *diagnostics_;
    MetatilesHeaderProvider metatiles_provider_;
    mutable std::optional<LayerValue<MetatileAttrFieldSpecs>> cached_result_;
};

} // namespace porytiles
