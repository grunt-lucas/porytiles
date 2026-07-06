#include "porytiles/domain/services/metatile_attr_schema_loader.hpp"

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

} // namespace

ChainableResult<LoadedAttrSchema> load_metatile_attr_schema(
    const MetatileAttrFieldSpecs &fields,
    const MetatileAttrFieldOverrides &overrides,
    std::size_t attr_bytes,
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
    std::vector<Field> schema_fields;

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

        if (merged.mask.has_value()) {
            schema_fields.push_back(
                Field{merged.name, merged.mask.value(), merged.default_value.value_or(0), merged.provider});
        }
        resolved.push_back(std::move(merged));
    }

    auto schema_result = Schema::create(std::move(schema_fields), attr_bytes);
    if (!schema_result.has_value()) {
        return ChainableResult<LoadedAttrSchema>{
            FormattableError{"the configured metatile attribute fields do not form a valid layout."}, schema_result};
    }

    return LoadedAttrSchema{std::move(schema_result).value(), std::move(resolved)};
}

} // namespace porytiles
