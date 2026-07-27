#include "porytiles/infra/services/metatile_attribute_schema_resolver.hpp"

#include <utility>

#include "porytiles/domain/algorithms/metatile_attribute_inference.hpp"
#include "porytiles/domain/algorithms/metatile_attribute_schema_reconciler.hpp"
#include "porytiles/infra/services/metatile_attribute_scanner.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"

namespace porytiles {

MetatileAttributeSchemaResolver::MetatileAttributeSchemaResolver(
    std::filesystem::path project_root,
    gsl::not_null<const InfraConfig *> config,
    gsl::not_null<const TextFormatter *> format,
    gsl::not_null<const UserDiagnostics *> diag)
    : project_root_{std::move(project_root)}, config_{config}, format_{format}, diag_{diag}
{
}

ChainableResult<LoadedMetatileAttributeSchema>
MetatileAttributeSchemaResolver::resolve(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        fields_cv,
        config_->metatile_attribute_fields(ConfigScopeType::tileset, tileset_name),
        LoadedMetatileAttributeSchema);
    PT_TRY_ASSIGN_PASS_ERR(
        overrides_cv,
        config_->metatile_attribute_field_overrides(ConfigScopeType::tileset, tileset_name),
        LoadedMetatileAttributeSchema);
    PT_TRY_ASSIGN_PASS_ERR(
        size_cv,
        config_->metatile_attribute_size(ConfigScopeType::tileset, tileset_name),
        LoadedMetatileAttributeSchema);
    PT_TRY_ASSIGN_PASS_ERR(
        declaration_cv,
        config_->metatile_attribute_declaration_size(ConfigScopeType::tileset, tileset_name),
        LoadedMetatileAttributeSchema);

    // Gather the project's raw fieldmap facts and run the inference process. Inference runs on whatever the scan
    // gathered, including nothing at all: a missing fieldmap header is not an error, and it simply reports that
    // nothing was found so the reconciler can fall back to the user's inputs. Running it even when the scan came up
    // empty is what lets a file that exists but could not be read be reported as such, rather than as a project that
    // declares no masks.
    MetatileAttributeScanner scanner{project_root_, format_, diag_};
    const auto scan = scanner.scan_project();

    const auto inference = infer_metatile_attribute_candidates(scan, format_, diag_);

    MetatileAttributeConfigInputs inputs;
    inputs.fields = fields_cv.value();
    inputs.fields_source = fields_cv.source();
    inputs.overrides = overrides_cv.value();
    inputs.attribute_size = size_cv.value();
    inputs.attribute_size_source = size_cv.source();
    inputs.declaration_size = declaration_cv.value();
    inputs.fieldmap_header_source = scan.header_source;

    return reconcile_metatile_attribute_schema(inference, inputs, format_, diag_);
}

} // namespace porytiles
