#include "porytiles/infra/services/metatile_attribute_schema_resolver.hpp"

#include <utility>

#include "porytiles/domain/algorithms/metatile_attribute_inference.hpp"
#include "porytiles/domain/algorithms/metatile_attribute_schema_reconciler.hpp"
#include "porytiles/infra/services/metatile_attribute_scanner.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"

namespace porytiles {

namespace {

constexpr const char *inference_tag = "metatile-attr-inference";
constexpr const char *schema_tag = "metatile-attr-schema";

} // namespace

MetatileAttributeSchemaResolver::MetatileAttributeSchemaResolver(
    std::filesystem::path project_root,
    gsl::not_null<const LazyLayeredConfig *> config,
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
        layer_mask_cv,
        config_->metatile_layer_type_mask(ConfigScopeType::tileset, tileset_name),
        LoadedMetatileAttributeSchema);
    PT_TRY_ASSIGN_PASS_ERR(
        size_cv,
        config_->metatile_attribute_size(ConfigScopeType::tileset, tileset_name),
        LoadedMetatileAttributeSchema);
    PT_TRY_ASSIGN_PASS_ERR(
        declaration_cv,
        config_->metatile_attribute_declaration_size(ConfigScopeType::tileset, tileset_name),
        LoadedMetatileAttributeSchema);

    // Gather the project's raw fieldmap facts and run the pure inference over them. A missing fieldmap header is not
    // an error: the default-constructed inference simply reports nothing, and reconciliation falls back to the
    // user's inputs and the documented assumptions.
    MetatileAttributeScanner scanner{project_root_, format_};
    const auto outcome = scanner.scan_project();
    for (const auto &warning : outcome.warnings) {
        diag_->warning(inference_tag, warning);
    }

    MetatileAttributeInferenceResult inference;
    if (outcome.fieldmap_present) {
        inference = infer_metatile_attribute_candidates(outcome.scan, format_);
        for (const auto &warning : inference.warnings) {
            diag_->warning(inference_tag, warning);
        }
    }

    MetatileAttributeConfigInputs inputs;
    inputs.fields = fields_cv.value();
    inputs.fields_source = fields_cv.source();
    inputs.overrides = overrides_cv.value();
    inputs.attribute_size = size_cv.value();
    inputs.attribute_size_source = size_cv.source();
    inputs.declaration_size = declaration_cv.value();
    inputs.layer_type_mask = layer_mask_cv.value();
    inputs.scan_source = outcome.source;

    auto reconciliation = reconcile_metatile_attribute_schema(inference, inputs, format_);

    // Drain the notes before checking the result: the assumed-width warning must reach the user on the error path
    // too, since it is decided before reconciliation can still fail.
    for (const auto &note : reconciliation.notes) {
        if (note.severity == AttributeNoteSeverity::warning) {
            diag_->warning(schema_tag, note.text);
        }
        else {
            diag_->remark(schema_tag, note.text);
        }
    }

    return std::move(reconciliation.result);
}

} // namespace porytiles
