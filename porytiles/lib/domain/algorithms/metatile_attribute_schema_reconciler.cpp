#include "porytiles/domain/algorithms/metatile_attribute_schema_reconciler.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

namespace {

[[nodiscard]] std::string join_names(const MetatileAttributeFieldSpecs &fields)
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

/// @brief Merges field overrides into a baseline field list and validates the merged specs.
///
/// @details
/// Applies the empty-list, unique-name, unknown-override, provider-override, and missing-mask checks, and returns the
/// fully merged specs. Overrides are applied additively: each present override member replaces the baseline value, a
/// present `skipped` set replaces the baseline skip set wholesale, and `provider` removal drops a field's provider.
[[nodiscard]] ChainableResult<MetatileAttributeFieldSpecs> merge_field_overrides(
    const MetatileAttributeFieldSpecs &fields,
    const MetatileAttributeFieldOverrides &overrides,
    gsl::not_null<const TextFormatter *> format)
{
    if (fields.empty()) {
        return FormattableError{std::vector<std::string>{
            "No metatile attribute fields are configured.",
            "Porytiles could not infer a field layout and none was provided.",
            "Add a metatile_attribute_fields list to your Porytiles config, or make sure the base game exposes "
            "its attribute masks (a METATILE_ATTR_*_MASK define or an sMetatileAttrMasks table)."}};
    }

    // A baseline name must be unique so overrides and the schema can address fields unambiguously.
    std::unordered_set<std::string> names;
    for (const auto &field : fields) {
        if (!names.insert(field.name).second) {
            return FormattableError{
                format->format("Field '{}' is defined more than once.", FormatParam{field.name, Style::bold})};
        }
    }

    // Every override must name an existing field.
    for (const auto &name : overrides | std::views::keys) {
        if (!names.contains(name)) {
            return FormattableError{std::vector<std::string>{
                format->format("Override names unknown field '{}'.", FormatParam{name, Style::bold}),
                format->format("Available fields: {}.", FormatParam{join_names(fields), Style::bold})}};
        }
    }

    MetatileAttributeFieldSpecs resolved;
    resolved.reserve(fields.size());

    for (const auto &baseline : fields) {
        MetatileAttributeFieldSpec merged = baseline;

        if (const auto it = overrides.find(baseline.name); it != overrides.end()) {
            const MetatileAttributeFieldOverride &override_value = it->second;
            if (override_value.mask.has_value()) {
                merged.mask = override_value.mask;
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
                            "Provider override for field '{}' must supply both a header and a prefix.",
                            FormatParam{baseline.name, Style::bold})};
                    }
                    merged.provider = std::move(provider);
                }
            }
        }

        if (!merged.mask.has_value()) {
            return FormattableError{std::vector<std::string>{
                format->format("Field '{}' has no mask.", FormatParam{merged.name, Style::bold}),
                "Every field must define one."}};
        }

        resolved.push_back(std::move(merged));
    }

    return resolved;
}

/// @brief Loads the schema from field specs: merge the overrides, size the word, and build the Schema.
///
/// @details
/// How the attribute byte width is decided depends on @p attribute_size_is_authoritative. When it is @c false the
/// given width was only an assumption, so the configured masks are the sole evidence of the true width and the word
/// is widened silently to the smallest of 1, 2, or 4 bytes that covers them, never below @p attribute_bytes. When it
/// is @c true the width came from a real source (an inferred mask set or an explicit config knob) and is fixed for
/// the whole project, so a mask that needs a wider word contradicts a hard fact and is a hard error rather than a
/// silent widen. Masks narrower than an authoritative width are fine; only exceeding it is an error.
[[nodiscard]] ChainableResult<LoadedMetatileAttributeSchema> load_metatile_attribute_schema(
    const MetatileAttributeFieldSpecs &fields,
    const MetatileAttributeFieldOverrides &overrides,
    std::size_t attribute_bytes,
    bool attribute_size_is_authoritative,
    std::optional<std::uint32_t> layer_type_mask,
    std::optional<std::size_t> declaration_size,
    gsl::not_null<const TextFormatter *> format)
{
    PT_TRY_ASSIGN_PASS_ERR(resolved, merge_field_overrides(fields, overrides, format), LoadedMetatileAttributeSchema);

    std::vector<Field> schema_fields;
    // The widest bit set by any field mask or by an explicit layer_type mask decides the minimum word width.
    // widest_source names whatever set that bit, so a conflict against an authoritative width can point at the
    // offending mask.
    std::size_t required_bits = 0;
    std::string widest_source;
    for (const auto &merged : resolved) {
        const auto bits = static_cast<std::size_t>(std::bit_width(merged.mask.value()));
        if (bits > required_bits) {
            required_bits = bits;
            widest_source = std::format("Field '{}' (mask 0x{:X})", merged.name, merged.mask.value());
        }
        schema_fields.push_back(
            Field{merged.name, merged.mask.value(), merged.default_value.value_or(0), merged.provider});
    }

    // An explicit non-zero layer_type mask is part of the layout too, so a wide one (e.g. 0x60000000) widens the word.
    // An unset (nullopt) mask resolves to the size-based default later, which always fits the chosen width by
    // definition.
    if (layer_type_mask.has_value() && layer_type_mask.value() != 0) {
        const auto bits = static_cast<std::size_t>(std::bit_width(layer_type_mask.value()));
        if (bits > required_bits) {
            required_bits = bits;
            widest_source = std::format("The layer-type mask 0x{:X}", layer_type_mask.value());
        }
    }

    const std::size_t mask_bytes = required_bits <= 8 ? 1U : (required_bits <= 16 ? 2U : 4U);

    if (attribute_size_is_authoritative && mask_bytes > attribute_bytes) {
        // An authoritative width (an inferred mask set or an explicit config knob) is not a guess: it IS the read
        // stride, fixed for the whole project and shared across every tileset (they all feed one
        // 'const uN *metatileAttributes'). A mask that needs a wider word contradicts that hard fact, so reject it
        // instead of silently widening, which would emit a mismatched declaration and corrupt the packed attributes
        // on the next read. Masks narrower than the width are fine (Porymap parity): unused high bits stay zero.
        return FormattableError{
            "{} needs a {}-byte attribute word, but the project's metatile attribute size is {} bytes. That size is "
            "fixed for the whole project and shared by every tileset, so Porytiles cannot widen it to fit this mask. "
            "Narrow the mask to fit {} bytes, or set 'fieldmap.metatile_attribute_size' (or pass "
            "--metatile-attribute-size) if the project really uses a wider attribute word.",
            FormatParam{widest_source, Style::bold},
            FormatParam{mask_bytes, Style::bold},
            FormatParam{attribute_bytes, Style::bold},
            FormatParam{attribute_bytes, Style::bold}};
    }
    // An assumed width is only an assumption, so the masks are the sole evidence of the true width: widen silently
    // to the smallest of 1, 2, or 4 bytes that covers them, but never below the given width.
    const std::size_t resolved_attribute_bytes = std::max(attribute_bytes, mask_bytes);

    // Generated C declarations follow the explicit declaration width when one was resolved (on expansion's FRLG
    // build that is narrower than the attribute width: 'const u16' arrays read as 4-byte words); otherwise they
    // match the attribute width.
    const std::size_t declaration_bytes = declaration_size.value_or(resolved_attribute_bytes);

    auto schema_result = Schema::create(std::move(schema_fields), resolved_attribute_bytes, layer_type_mask);
    if (!schema_result.has_value()) {
        return ChainableResult<LoadedMetatileAttributeSchema>{
            FormattableError{"The configured metatile attribute fields do not form a valid layout."}, schema_result};
    }

    return LoadedMetatileAttributeSchema{
        std::move(schema_result).value(), std::move(resolved), resolved_attribute_bytes, declaration_bytes, {}, {}, {}};
}

// Renders a candidate list for selection errors: "{origin} ({required_bytes} bytes), ...".
[[nodiscard]] std::string describe_candidates(const std::vector<MetatileAttributeCandidateSet> &candidates)
{
    std::string described;
    for (const auto &candidate : candidates) {
        if (!described.empty()) {
            described += ", ";
        }
        described += std::format("{} ({} bytes)", candidate.origin, candidate.required_bytes);
    }
    return described;
}

// Returns pointers to the inferred sets whose required width equals attribute_size. The pointers alias
// candidates, so it must outlive them.
[[nodiscard]] std::vector<const MetatileAttributeCandidateSet *>
sets_matching_size(const std::vector<MetatileAttributeCandidateSet> &candidates, std::size_t attribute_size)
{
    std::vector<const MetatileAttributeCandidateSet *> matches;
    for (const auto &candidate : candidates) {
        if (candidate.required_bytes == attribute_size) {
            matches.push_back(&candidate);
        }
    }
    return matches;
}

} // namespace

MetatileAttributeReconciliation reconcile_metatile_attribute_schema(
    const MetatileAttributeInferenceResult &inference,
    const MetatileAttributeConfigInputs &inputs,
    gsl::not_null<const TextFormatter *> format)
{
    std::vector<AttributeReconcileNote> notes;

    // Step 1: decide the width and whether it is authoritative. Only non-synthesized candidate sets can pin the
    // width: a synthesized set's 2 bytes are an assumption, not a project fact, so behavior-only projects fall
    // through to the assumed default and its warning exactly as when no masks exist at all.
    std::vector<const MetatileAttributeCandidateSet *> real_sets;
    if (inference.status == AttributeInferenceStatus::valid) {
        for (const auto &candidate : inference.candidates) {
            if (!candidate.synthesized) {
                real_sets.push_back(&candidate);
            }
        }
    }

    std::size_t attribute_size = 2;
    bool width_is_authoritative = false;
    std::string size_origin;
    if (inputs.attribute_size.has_value()) {
        attribute_size = inputs.attribute_size.value();
        width_is_authoritative = true;
        size_origin = inputs.attribute_size_source;
    }
    else if (real_sets.size() == 1) {
        attribute_size = real_sets.front()->required_bytes;
        width_is_authoritative = true;
        size_origin = std::format("inferred from {} ({})", real_sets.front()->origin, inputs.scan_source);
    }
    else if (real_sets.size() >= 2) {
        // More than one mask layout (stock pokeemerald-expansion): the source tree holds both build flavors and no
        // project file records which one the user targets (it is a make argument), so the size cannot be
        // reconciled. Fatal even when the fields are explicit: the width is the read stride, and guessing it wrong
        // silently corrupts every attribute the project compiles.
        return {
            ChainableResult<LoadedMetatileAttributeSchema>{FormattableError{format->format(
                "Porytiles found more than one metatile attribute mask layout in this project ({} ({} bytes) and {} "
                "({} bytes)), so it cannot infer the attribute size. Set 'fieldmap.metatile_attribute_size' in "
                "porytiles/config.yaml (or pass --metatile-attribute-size) to choose the layout this build uses.",
                FormatParam{real_sets.at(0)->origin, Style::bold},
                FormatParam{real_sets.at(0)->required_bytes},
                FormatParam{real_sets.at(1)->origin, Style::bold},
                FormatParam{real_sets.at(1)->required_bytes})}},
            std::move(notes)};
    }
    else {
        // A defaulted width silently landing on 2 bytes could halve a real 4-byte project's attribute layout when
        // its configured masks all sit below bit 16 (masks can widen the layout but never prove it narrow), so say
        // what was assumed and how to pin the width.
        size_origin = "assumed (no explicit knob and no inferable mask layout)";
        notes.push_back(
            AttributeReconcileNote{
                AttributeNoteSeverity::warning,
                format->format(
                    "Porytiles could not determine the metatile attribute size for this project and assumed {}-byte "
                    "attributes. It normally infers the size from the attribute masks the project declares (the "
                    "METATILE_ATTR_*_MASK defines in 'include/global.fieldmap.h' or the sMetatileAttrMasks table in "
                    "'src/fieldmap.c'); set 'fieldmap.metatile_attribute_size' in porytiles/config.yaml (or pass "
                    "--metatile-attribute-size) to pin the width. Field masks can widen an assumed width but never "
                    "prove it narrow, so a 4-byte project with only low-bit masks needs the width pinned.",
                    FormatParam{attribute_size})});
    }

    // Step 2: decide the field set. Explicit fields are the truth; otherwise select the inferred candidate set whose
    // required width equals the resolved size. Either way, selected names the set the project's own layout came
    // from, so the layer-type mask can be read off it below. Selection runs over ALL candidates including
    // synthesized ones: a synthesized set cannot pin the width, but it is still the layout the project uses.
    MetatileAttributeFieldSpecs fields = inputs.fields;
    std::string fields_origin;
    const MetatileAttributeCandidateSet *selected = nullptr;
    if (!fields.empty()) {
        fields_origin = std::format("explicit metatile_attribute_fields ({})", inputs.fields_source);
        // Declaring fields explicitly is the documented way out of masks Porytiles cannot infer: both inference
        // failures name this key as the remedy. So selection is advisory here: an invalid inference, no width
        // match, or an ambiguous match simply leaves selected null.
        if (inference.status == AttributeInferenceStatus::valid) {
            const auto matches = sets_matching_size(inference.candidates, attribute_size);
            if (matches.size() == 1) {
                selected = matches.front();
            }
        }
    }
    else if (inference.status == AttributeInferenceStatus::invalid) {
        // The masks themselves are unusable (e.g. an undecidable conditional), and there are no explicit fields to
        // fall back on, so resolution cannot proceed even when an explicit size rescued the width.
        return {
            ChainableResult<LoadedMetatileAttributeSchema>{FormattableError{inference.error_message}},
            std::move(notes)};
    }
    else if (!inference.candidates.empty()) {
        const auto matches = sets_matching_size(inference.candidates, attribute_size);
        if (matches.empty()) {
            // This error can only fire when the user pinned the width: an unpinned width either came from the
            // unique candidate itself (which then matches) or was assumed at 2 (which matches the synthesized
            // set). So interpolating the size's config source stays meaningful.
            return {
                ChainableResult<LoadedMetatileAttributeSchema>{FormattableError{format->format(
                    "Porytiles inferred these metatile attribute mask layouts from the project: {}. None of them "
                    "fits the resolved attribute size of {} bytes (from {}). Change "
                    "'fieldmap.metatile_attribute_size' (or --metatile-attribute-size) to one of the listed "
                    "widths, or declare metatile_attribute_fields in your Porytiles config to define the layout "
                    "explicitly.",
                    FormatParam{describe_candidates(inference.candidates), Style::bold},
                    FormatParam{attribute_size, Style::bold},
                    FormatParam{inputs.attribute_size_source, Style::bold})}},
                std::move(notes)};
        }
        if (matches.size() > 1) {
            return {
                ChainableResult<LoadedMetatileAttributeSchema>{FormattableError{format->format(
                    "Porytiles inferred more than one metatile attribute mask layout matching the resolved "
                    "attribute size of {} bytes: {}. It cannot choose between them. Declare "
                    "metatile_attribute_fields in your Porytiles config to define the layout explicitly.",
                    FormatParam{attribute_size, Style::bold},
                    FormatParam{describe_candidates(inference.candidates), Style::bold})}},
                std::move(notes)};
        }
        selected = matches.front();
        fields = selected->fields;
        fields_origin = selected->origin;
        notes.push_back(
            AttributeReconcileNote{
                AttributeNoteSeverity::remark,
                format->format(
                    "Porytiles selected the metatile attribute mask layout inferred from {} ({} bytes).",
                    FormatParam{selected->origin, Style::bold},
                    FormatParam{selected->required_bytes})});
    }
    // With no candidates the field list stays empty and the merge below reports the no-fields-configured error.

    // Step 3: the inferred layout's layer-type mask applies unless the user set one explicitly. This is deliberately
    // independent of where the fields came from: a project that moves its layer-type bits keeps them there even when
    // it declares its fields explicitly, and those are unrelated knobs. A null selected (no usable inference, or no
    // unique width match) leaves the mask unset, and Schema::create falls back to the size-based default.
    std::optional<std::uint32_t> layer_mask = inputs.layer_type_mask;
    if (!layer_mask.has_value() && selected != nullptr) {
        layer_mask = selected->layer_type_mask;
    }

    // Step 5 precedence, decided before the merge so the loader receives the final choice: the explicit knob beats
    // the width inferred from struct Tileset's declaration, which beats the post-widening attribute width (the
    // loader's fallback when this stays nullopt).
    std::optional<std::size_t> declaration_size = inputs.declaration_size;
    std::string declaration_origin = "explicit metatile_attribute_declaration_size";
    if (!declaration_size.has_value()) {
        declaration_size = inference.declaration_size;
        declaration_origin =
            declaration_size.has_value()
                ? std::format("inferred from struct Tileset's metatileAttributes member ({})", inputs.scan_source)
                : "matches the resolved attribute size";
    }

    // Step 4: merge the overrides, widen or reject against the resolved width, and build the schema.
    auto loaded_result = load_metatile_attribute_schema(
        fields, inputs.overrides, attribute_size, width_is_authoritative, layer_mask, declaration_size, format);
    if (!loaded_result.has_value()) {
        return {std::move(loaded_result), std::move(notes)};
    }
    LoadedMetatileAttributeSchema loaded = std::move(loaded_result).value();
    loaded.fields_origin = std::move(fields_origin);
    loaded.size_origin = std::move(size_origin);
    loaded.declaration_origin = std::move(declaration_origin);

    // Summarize the resolved schema so the user can see what layout the data-driven resolution landed on.
    std::string field_names;
    for (const Field &field : loaded.schema.fields()) {
        if (!field_names.empty()) {
            field_names += ", ";
        }
        field_names += field.name();
    }
    const std::uint32_t resolved_layer_mask = loaded.schema.layer_type_mask();
    const std::string layer_type_note =
        resolved_layer_mask == 0 ? "layer type disabled" : std::format("layer type mask 0x{:X}", resolved_layer_mask);
    notes.push_back(
        AttributeReconcileNote{
            AttributeNoteSeverity::remark,
            format->format(
                "Porytiles resolved {}-byte metatile attributes with fields: {} ({}).",
                FormatParam{loaded.attribute_bytes, Style::bold},
                FormatParam{field_names, Style::bold},
                FormatParam{layer_type_note, Style::bold})});

    return {std::move(loaded), std::move(notes)};
}

} // namespace porytiles
