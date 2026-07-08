#include "porytiles/infra/services/tileset_attr_schema_resolver.hpp"

#include <cstdint>
#include <format>
#include <optional>

#include "porytiles/infra/config/frlg_alternate_mask_mode.hpp"
#include "porytiles/infra/config/layer_value.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"

namespace porytiles {

TilesetAttrSchemaResolver::TilesetAttrSchemaResolver(
    gsl::not_null<const LazyLayeredConfig *> config,
    gsl::not_null<const ProjectLayoutMetadataProvider *> layout_metadata,
    gsl::not_null<const MetatilesHeaderProvider *> metatiles,
    gsl::not_null<const TextFormatter *> format,
    gsl::not_null<const UserDiagnostics *> diag)
    : config_{config}, layout_metadata_{layout_metadata}, metatiles_{metatiles}, format_{format}, diag_{diag}
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
        frlg_mode_cv,
        config_->use_frlg_alternate_masks(ConfigScopeType::tileset, tileset_name),
        ResolvedTilesetAttrSchema);
    PT_TRY_ASSIGN_PASS_ERR(
        layer_mask_cv,
        config_->metatile_layer_type_mask(ConfigScopeType::tileset, tileset_name),
        ResolvedTilesetAttrSchema);
    PT_TRY_ASSIGN_PASS_ERR(
        layer_mask_frlg_cv,
        config_->metatile_layer_type_mask_frlg(ConfigScopeType::tileset, tileset_name),
        ResolvedTilesetAttrSchema);

    const MetatileAttrFieldSpecs fields = fields_cv.value();
    const MetatileAttrFieldOverrides overrides = overrides_cv.value();
    const FrlgAlternateMaskMode frlg_mode = frlg_mode_cv.value();
    const std::optional<std::uint32_t> layer_mask_primary = layer_mask_cv.value();
    const std::optional<std::uint32_t> layer_mask_frlg = layer_mask_frlg_cv.value();

    // The attribute byte width comes straight from the metatiles.h detector. A project with no detectable width (no
    // metatiles.h, or no attribute declarations in it) defaults to 2 bytes; mixed u16/u32 declarations are a hard
    // error. The resolved schema may still widen past the detected width to cover the selected masks, and the frlg
    // layout ignores the detected width entirely (its entry width is always 4 bytes, the engine's read stride).
    const LayerValue<std::size_t> detected = metatiles_->detect();
    if (detected.state == ValidationState::invalid) {
        return FormattableError{detected.error_message};
    }
    // An undetectable width silently landing on 2 bytes could halve a real 4-byte project's attribute layout when its
    // configured masks all sit below bit 16 (masks can widen the layout but never prove it narrow), so say what was
    // assumed and how to pin the width.
    if (detected.state != ValidationState::valid) {
        diag_->warning(
            "metatile-attr-schema",
            "Porytiles could not detect the metatile attribute size from 'src/data/tilesets/metatiles.h' for tileset "
            "'{}' and assumed {}-byte attributes. Declare gMetatileAttributes_* as 'const u8', 'const u16', or "
            "'const u32' in metatiles.h to pin the width (1, 2, or 4 bytes), or configure a field whose mask needs a "
            "wider word.",
            FormatParam{tileset_name, Style::bold},
            FormatParam{2});
    }
    const bool detected_width_is_authoritative = detected.state == ValidationState::valid;
    const std::size_t detected_attr_bytes = detected_width_is_authoritative ? detected.value.value() : std::size_t{2};

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
                    "Porytiles could not read 'data/layouts/layouts.json' to determine FRLG-ness for tileset '{}' and "
                    "assumed the primary attribute layout. Set use_frlg_alternate_masks explicitly to silence this.",
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
                "Tileset '{}' is referenced only by frlg layouts in 'data/layouts/layouts.json', so Porytiles "
                "selected the FRLG alternate metatile attribute masks.",
                FormatParam{tileset_name, Style::bold});
            break;
        case TilesetLayoutVersionUsage::emerald_only:
        case TilesetLayoutVersionUsage::unreferenced:
            layout = AttrSchemaLayout::primary;
            break;
        case TilesetLayoutVersionUsage::mixed:
            return FormattableError{format_->format(
                "Tileset '{}' is referenced by both emerald and frlg layouts in 'data/layouts/layouts.json', so its "
                "attribute schema is ambiguous. Set use_frlg_alternate_masks (always or never) in "
                "porytiles/tilesets/{}/config.yaml to choose.",
                FormatParam{tileset_name, Style::bold},
                FormatParam{tileset_name, Style::bold})};
        }
        break;
    }
    }

    // The layer-type mask follows the same primary/FRLG selection as the fields: the FRLG value for the FRLG layout,
    // the primary value otherwise. An unset (nullopt) value lets the size-based default apply in Schema::create.
    const std::optional<std::uint32_t> layer_mask_opt =
        layout == AttrSchemaLayout::frlg ? layer_mask_frlg : layer_mask_primary;

    auto resolved = resolve_tileset_attr_schema(
        fields, overrides, layout, detected_attr_bytes, detected_width_is_authoritative, layer_mask_opt, format_);
    if (resolved.has_value()) {
        // A declared width narrower than 4 is expected on expansion (FRLG tilesets declared 'const u16' but read as
        // 4-byte words), yet surprising enough to be worth a remark when the frlg layout overrides it.
        if (layout == AttrSchemaLayout::frlg && detected_width_is_authoritative && detected_attr_bytes != 4) {
            diag_->remark(
                "metatile-attr-schema",
                "Although 'src/data/tilesets/metatiles.h' declares {}-byte attributes, tileset '{}' uses the FRLG "
                "attribute layout, which the engine reads as 4-byte words at runtime, so Porytiles resolved 4-byte "
                "attributes.",
                FormatParam{detected_attr_bytes, Style::bold},
                FormatParam{tileset_name, Style::bold});
        }
        // Summarize the resolved schema so the user can see what layout the data-driven resolution landed on. This is
        // the schema-shaped replacement for the old "detected base game" remark.
        std::string field_names;
        for (const Field &field : resolved.value().schema.fields()) {
            if (!field_names.empty()) {
                field_names += ", ";
            }
            field_names += field.name();
        }
        const std::uint32_t resolved_layer_mask = resolved.value().schema.layer_type_mask();
        const std::string layer_type_note = resolved_layer_mask == 0
                                                ? "layer type disabled"
                                                : std::format("layer type mask 0x{:X}", resolved_layer_mask);
        diag_->remark(
            "metatile-attr-schema",
            "Porytiles resolved {}-byte metatile attributes for tileset '{}' with fields: {} ({}).",
            FormatParam{resolved.value().attr_bytes, Style::bold},
            FormatParam{tileset_name, Style::bold},
            FormatParam{field_names, Style::bold},
            FormatParam{layer_type_note, Style::bold});
    }
    return resolved;
}

} // namespace porytiles
