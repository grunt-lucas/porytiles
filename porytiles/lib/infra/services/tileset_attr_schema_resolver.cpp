#include "porytiles/infra/services/tileset_attr_schema_resolver.hpp"

#include "porytiles/infra/config/frlg_alternate_mask_mode.hpp"
#include "porytiles/infra/config/layer_value.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"

namespace porytiles {

TilesetAttrSchemaResolver::TilesetAttrSchemaResolver(
    gsl::not_null<const LazyLayeredConfig *> config,
    gsl::not_null<const ProjectLayoutMetadataProvider *> layout_metadata,
    gsl::not_null<const TextFormatter *> format,
    gsl::not_null<const UserDiagnostics *> diag)
    : config_{config}, layout_metadata_{layout_metadata}, format_{format}, diag_{diag}
{
}

ChainableResult<ResolvedTilesetAttrSchema> TilesetAttrSchemaResolver::resolve(const std::string &tileset_name) const
{
    PT_TRY_ASSIGN_PASS_ERR(
        fields_cv, config_->metatile_attr_fields(ConfigScopeType::tileset, tileset_name), ResolvedTilesetAttrSchema);
    PT_TRY_ASSIGN_PASS_ERR(
        overrides_cv,
        config_->metatile_attr_field_overrides(ConfigScopeType::tileset, tileset_name),
        ResolvedTilesetAttrSchema);
    PT_TRY_ASSIGN_PASS_ERR(
        attr_size_cv, config_->metatile_attr_size(ConfigScopeType::tileset, tileset_name), ResolvedTilesetAttrSchema);
    PT_TRY_ASSIGN_PASS_ERR(
        frlg_mode_cv,
        config_->use_frlg_alternate_masks(ConfigScopeType::tileset, tileset_name),
        ResolvedTilesetAttrSchema);

    const MetatileAttrFieldSpecs fields = fields_cv.value();
    const MetatileAttrFieldOverrides overrides = overrides_cv.value();
    const std::size_t configured_attr_bytes = attr_size_cv.value();
    const FrlgAlternateMaskMode frlg_mode = frlg_mode_cv.value();

    // Attr size is "explicit" only when the user set it via the CLI or a YAML file. A value synthesized by the
    // metatiles-header or default provider does not count, so an FRLG layout may widen it silently. Walk the provenance
    // chain and let the first provider that actually provides a value (valid or invalid) decide.
    bool attr_bytes_explicit = false;
    for (const auto &link : config_->metatile_attr_size_provenance_chain(ConfigScopeType::tileset, tileset_name)) {
        if (link.layer_value.state == ValidationState::not_provided) {
            continue;
        }
        attr_bytes_explicit = link.provider_name == "CliOptionProvider" || link.provider_name == "YamlFileProvider";
        break;
    }

    AttrSchemaLayout layout = AttrSchemaLayout::primary;
    switch (frlg_mode) {
    case FrlgAlternateMaskMode::always:
        layout = AttrSchemaLayout::frlg;
        break;
    case FrlgAlternateMaskMode::never:
        layout = AttrSchemaLayout::primary;
        break;
    case FrlgAlternateMaskMode::automatic: {
        auto usage_result = layout_metadata_->layout_version_usage(tileset_name);
        if (!usage_result.has_value()) {
            // An error here means layouts.json exists (an absent file yields unreferenced, not an error). Distinguish a
            // malformed file from a typo'd layout_version value: a plain parse fails for the former but not the latter.
            // layout_names() triggers (cached) parsing, so its success proves the file is well-formed.
            if (!layout_metadata_->layout_names().has_value()) {
                diag_->warning(
                    "frlg-alternate-masks",
                    "could not read 'data/layouts/layouts.json' to determine FRLG-ness for tileset '{}'; assuming the "
                    "primary attribute layout. Set use_frlg_alternate_masks explicitly to silence this.",
                    FormatParam{tileset_name, Style::bold});
                layout = AttrSchemaLayout::primary;
                break;
            }
            // The file parses, so the error is the invalid layout_version value. Surface it as fatal.
            return ChainableResult<ResolvedTilesetAttrSchema>{FormattableError{}, usage_result};
        }

        switch (usage_result.value()) {
        case TilesetLayoutVersionUsage::frlg_only:
            layout = AttrSchemaLayout::frlg;
            diag_->remark(
                "frlg-alternate-masks",
                "tileset '{}' is referenced only by frlg layouts in data/layouts/layouts.json; selecting the FRLG "
                "alternate metatile attribute masks.",
                FormatParam{tileset_name, Style::bold});
            break;
        case TilesetLayoutVersionUsage::emerald_only:
        case TilesetLayoutVersionUsage::unreferenced:
            layout = AttrSchemaLayout::primary;
            break;
        case TilesetLayoutVersionUsage::mixed:
            return FormattableError{format_->format(
                "tileset '{}' is referenced by both emerald and frlg layouts in data/layouts/layouts.json, so its "
                "attribute schema is ambiguous. Set use_frlg_alternate_masks (always or never) in "
                "porytiles/tilesets/{}/config.yaml to choose.",
                FormatParam{tileset_name, Style::bold},
                FormatParam{tileset_name, Style::bold})};
        }
        break;
    }
    }

    return resolve_tileset_attr_schema(fields, overrides, layout, configured_attr_bytes, attr_bytes_explicit, format_);
}

} // namespace porytiles
