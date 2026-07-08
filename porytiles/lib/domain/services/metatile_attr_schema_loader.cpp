#include "porytiles/domain/services/metatile_attr_schema_loader.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

namespace {

[[nodiscard]] std::string join_names(const MetatileAttrFieldSpecs &fields)
{
    std::string joined;
    bool first = true;
    for (const auto &field : fields) {
        if (!first) {
            joined += ", ";
        }
        joined += field.name;
        first = false;
    }
    return joined;
}

/**
 * @brief Merges field overrides into a baseline field list and validates the merged specs.
 *
 * @details
 * Shared front half of load_metatile_attr_schema and resolve_tileset_attr_schema: applies the empty-list, unique-name,
 * unknown-override, provider-override, and neither-mask-nor-frlg_mask checks, and returns the fully merged specs. It
 * does not build a Schema, because which mask each spec contributes depends on the target layout, which is the caller's
 * concern.
 */
[[nodiscard]] ChainableResult<MetatileAttrFieldSpecs> merge_field_overrides(
    const MetatileAttrFieldSpecs &fields,
    const MetatileAttrFieldOverrides &overrides,
    gsl::not_null<const TextFormatter *> format)
{
    if (fields.empty()) {
        return FormattableError{
            "no metatile attribute fields are configured. Porytiles could not infer a field layout and none was "
            "provided. Add a metatile_attr_fields list to your Porytiles config, or make sure the base game exposes "
            "its attribute masks (a METATILE_ATTR_*_MASK define or an sMetatileAttrMasks table)."};
    }

    // A baseline name must be unique so overrides and the schema can address fields unambiguously.
    std::unordered_set<std::string> names;
    for (const auto &field : fields) {
        if (!names.insert(field.name).second) {
            return FormattableError{
                format->format("field '{}' is defined more than once.", FormatParam{field.name, Style::bold})};
        }
    }

    // Every override must name an existing field.
    for (const auto &[name, override_value] : overrides) {
        if (!names.contains(name)) {
            return FormattableError{format->format(
                "override names unknown field '{}'. Available fields: {}.",
                FormatParam{name, Style::bold},
                FormatParam{join_names(fields), Style::bold})};
        }
    }

    MetatileAttrFieldSpecs resolved;
    resolved.reserve(fields.size());

    for (const auto &baseline : fields) {
        MetatileAttrFieldSpec merged = baseline;

        if (const auto it = overrides.find(baseline.name); it != overrides.end()) {
            const MetatileAttrFieldOverride &override_value = it->second;
            if (override_value.mask.has_value()) {
                merged.mask = override_value.mask;
            }
            if (override_value.frlg_mask.has_value()) {
                merged.frlg_mask = override_value.frlg_mask;
            }
            if (override_value.default_value.has_value()) {
                merged.default_value = override_value.default_value;
            }
            if (override_value.provider.has_value()) {
                const ProviderSpecOverride &provider_override = override_value.provider.value();
                if (provider_override.remove) {
                    merged.provider = std::nullopt;
                }
                else {
                    ProviderSpec provider = merged.provider.value_or(ProviderSpec{});
                    if (provider_override.header.has_value()) {
                        provider.header = provider_override.header.value();
                    }
                    if (provider_override.prefix.has_value()) {
                        provider.prefix = provider_override.prefix.value();
                    }
                    if (provider_override.skipped.has_value()) {
                        provider.skipped = provider_override.skipped.value(); // replaces the base skip set wholesale
                    }
                    if (provider_override.format.has_value()) {
                        provider.format = provider_override.format.value();
                    }
                    if (provider.header.empty() || provider.prefix.empty()) {
                        return FormattableError{format->format(
                            "provider override for field '{}' must supply both a header and a prefix.",
                            FormatParam{baseline.name, Style::bold})};
                    }
                    merged.provider = std::move(provider);
                }
            }
        }

        if (!merged.mask.has_value() && !merged.frlg_mask.has_value()) {
            return FormattableError{format->format(
                "field '{}' has neither a mask nor a frlg_mask; a field must define at least one.",
                FormatParam{merged.name, Style::bold})};
        }

        resolved.push_back(std::move(merged));
    }

    return resolved;
}

} // namespace

ChainableResult<ResolvedTilesetAttrSchema> resolve_tileset_attr_schema(
    const MetatileAttrFieldSpecs &fields,
    const MetatileAttrFieldOverrides &overrides,
    AttrSchemaLayout layout,
    std::size_t detected_attr_bytes,
    std::optional<std::uint32_t> layer_type_mask,
    gsl::not_null<const TextFormatter *> format)
{
    PT_TRY_ASSIGN_PASS_ERR(resolved, merge_field_overrides(fields, overrides, format), ResolvedTilesetAttrSchema);

    const bool frlg = layout == AttrSchemaLayout::frlg;

    std::vector<Field> schema_fields;
    // The widest bit set by any selected field mask or by an explicit layer_type mask decides the minimum word width.
    std::size_t required_bits = 0;
    for (const auto &merged : resolved) {
        const std::optional<std::uint32_t> selected = frlg ? merged.frlg_mask : merged.mask;
        if (!selected.has_value()) {
            continue; // symmetric exclusion: a spec with no mask for the selected layout is dropped
        }
        required_bits = std::max(required_bits, static_cast<std::size_t>(std::bit_width(selected.value())));
        schema_fields.push_back(
            Field{merged.name, selected.value(), merged.default_value.value_or(0), merged.provider});
    }

    if (schema_fields.empty()) {
        if (frlg) {
            return FormattableError{
                "the FRLG attribute layout has no fields: none of the configured fields define a frlg_mask. Add a "
                "frlg_mask to at least one field, or set use_frlg_alternate_masks: never to use the primary masks."};
        }
        return FormattableError{
            "the primary attribute layout has no fields: none of the configured fields define a mask. Add a mask to at "
            "least one field."};
    }

    // An explicit non-zero layer_type mask is part of the layout too, so a wide one (e.g. 0x60000000) widens the word.
    // An unset (nullopt) mask resolves to the size convention later, which always fits the chosen width by definition.
    if (layer_type_mask.has_value() && layer_type_mask.value() != 0) {
        required_bits = std::max(required_bits, static_cast<std::size_t>(std::bit_width(layer_type_mask.value())));
    }

    // Widen silently to the smallest of 1, 2, or 4 bytes that covers the selected masks, but never below the width
    // detected from the project's own metatiles.h declarations.
    const std::size_t mask_bytes = required_bits <= 8 ? 1U : (required_bits <= 16 ? 2U : 4U);
    const std::size_t attr_bytes = std::max(detected_attr_bytes, mask_bytes);

    auto schema_result = Schema::create(std::move(schema_fields), attr_bytes, layer_type_mask);
    if (!schema_result.has_value()) {
        return ChainableResult<ResolvedTilesetAttrSchema>{
            FormattableError{"the configured metatile attribute fields do not form a valid layout."}, schema_result};
    }

    return ResolvedTilesetAttrSchema{std::move(schema_result).value(), std::move(resolved), layout, attr_bytes};
}

} // namespace porytiles
