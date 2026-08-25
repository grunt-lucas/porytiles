#include "porytiles/domain/algorithms/metatile_attribute_schema_reconciler.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

namespace {

// Names a field's role for diff prose. Written to read as the object of "has X", so a role-less field says so in
// words rather than leaving the reader to infer it from the other side of the comparison.
[[nodiscard]] std::string describe_role(const std::optional<FieldRole> &role)
{
    if (!role.has_value()) {
        return "no role";
    }
    return std::format("the {} role", to_string(role.value()));
}

[[nodiscard]] std::string join_names(const MetatileAttributeFieldDefinitions &fields)
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

/// @brief Merges field overrides into a baseline field list and validates the merged definitions.
///
/// @details
/// Applies the unique-name, unknown-override, provider-override, and missing-mask checks, and returns the fully
/// merged definitions. Overrides are applied additively: each present override member replaces the baseline value, a
/// present `skipped` set replaces the baseline skip set wholesale, `provider` removal drops a field's provider, and
/// a present role override sets or clears the field's role. The caller guarantees a non-empty baseline.
[[nodiscard]] ChainableResult<MetatileAttributeFieldDefinitions> merge_field_overrides(
    const MetatileAttributeFieldDefinitions &fields,
    const MetatileAttributeFieldOverrides &overrides,
    gsl::not_null<const TextFormatter *> format)
{
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

    MetatileAttributeFieldDefinitions resolved;
    resolved.reserve(fields.size());

    for (const auto &baseline : fields) {
        MetatileAttributeFieldDefinition merged = baseline;

        if (const auto it = overrides.find(baseline.name); it != overrides.end()) {
            const MetatileAttributeFieldOverride &override_value = it->second;
            if (override_value.mask.has_value()) {
                merged.mask = override_value.mask;
            }
            if (override_value.default_value.has_value()) {
                merged.default_value = override_value.default_value;
            }
            if (override_value.role.has_value()) {
                merged.role = override_value.role.value(); // an inner nullopt encodes `role: null`, clearing it
            }
            if (override_value.provider.has_value()) {
                const ProviderDefinitionOverride &provider_override = override_value.provider.value();
                if (provider_override.remove) {
                    merged.provider = std::nullopt;
                }
                else {
                    ProviderDefinition provider = merged.provider.value_or(ProviderDefinition{});
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

// The width evidence carried by a merged field list: the smallest of 1, 2, or 4 bytes covering every mask, plus a
// human-readable name for whichever field set the widest bit, so width errors can point at the offending mask.
struct RequiredWidth {
    std::size_t bytes{1};
    std::string widest_source;
};

[[nodiscard]] RequiredWidth required_width(const MetatileAttributeFieldDefinitions &resolved)
{
    std::size_t required_bits = 0;
    RequiredWidth width;
    for (const auto &field : resolved) {
        const auto bits = static_cast<std::size_t>(std::bit_width(field.mask.value()));
        if (bits > required_bits) {
            required_bits = bits;
            width.widest_source = std::format("Field '{}' (mask 0x{:X})", field.name, field.mask.value());
        }
    }
    width.bytes = required_bits <= 8 ? 1U : (required_bits <= 16 ? 2U : 4U);
    return width;
}

/// @brief Builds the validated Schema from fully merged field definitions.
[[nodiscard]] ChainableResult<Schema>
build_schema(const MetatileAttributeFieldDefinitions &resolved, std::size_t attribute_bytes)
{
    std::vector<Field> schema_fields;
    schema_fields.reserve(resolved.size());
    for (const auto &merged : resolved) {
        schema_fields.push_back(
            Field{merged.name, merged.mask.value(), merged.default_value.value_or(0), merged.provider, merged.role});
    }
    auto schema_result = Schema::create(std::move(schema_fields), attribute_bytes);
    if (!schema_result.has_value()) {
        return ChainableResult<Schema>{
            FormattableError{"The configured metatile attribute fields do not form a valid layout."}, schema_result};
    }
    return schema_result;
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

// Renders the width provenance of a selected inferred layout. The paths come from the candidate rather than from the
// fieldmap header, since a layout's masks may have been read from the src/fieldmap.c table instead (pokefirered
// declares no mask defines at all).
[[nodiscard]] std::string describe_inferred_size_origin(const MetatileAttributeCandidateSet &selected)
{
    if (selected.source.empty()) {
        return std::format("inferred from {}", selected.origin);
    }
    return std::format("inferred from {} ({})", selected.origin, selected.source);
}

// The candidate sets a pinned width can hold, split into exact width matches and narrower fits. The pointers alias
// the candidates vector, so it must outlive them.
struct SizeMatches {
    std::vector<const MetatileAttributeCandidateSet *> exact;
    std::vector<const MetatileAttributeCandidateSet *> narrower;
};

[[nodiscard]] SizeMatches
match_candidates_to_size(const std::vector<MetatileAttributeCandidateSet> &candidates, std::size_t attribute_size)
{
    SizeMatches matches;
    for (const auto &candidate : candidates) {
        if (candidate.required_bytes == attribute_size) {
            matches.exact.push_back(&candidate);
        }
        else if (candidate.required_bytes < attribute_size) {
            matches.narrower.push_back(&candidate);
        }
    }
    return matches;
}

/// @brief Renders a compact diff between explicit field definitions and an inferred layout.
///
/// @details
/// Compares names, masks, and roles. Defaults and providers are deliberately ignored: those are user territory that
/// inference merely guesses at, so differing there is not a disagreement with the source. Field order is ignored
/// too, since column order is a user preference rather than a layout fact. Returns an empty string when the layouts
/// agree.
[[nodiscard]] std::string describe_field_diff(
    const MetatileAttributeFieldDefinitions &explicit_fields, const MetatileAttributeFieldDefinitions &inferred)
{
    std::unordered_map<std::string, const MetatileAttributeFieldDefinition *> inferred_by_name;
    for (const auto &field : inferred) {
        inferred_by_name.emplace(field.name, &field);
    }
    std::unordered_set<std::string> explicit_names;
    for (const auto &field : explicit_fields) {
        explicit_names.insert(field.name);
    }

    std::vector<std::string> diffs;
    for (const auto &field : explicit_fields) {
        const auto it = inferred_by_name.find(field.name);
        if (it == inferred_by_name.end()) {
            diffs.push_back(std::format("'{}' is only in the config", field.name));
            continue;
        }
        const MetatileAttributeFieldDefinition &other = *it->second;
        if (field.mask != other.mask) {
            diffs.push_back(
                std::format(
                    "'{}' has mask {} in the config but {} in the source",
                    field.name,
                    detail::format_optional_mask(field.mask),
                    detail::format_optional_mask(other.mask)));
        }
        if (field.role != other.role) {
            diffs.push_back(
                std::format(
                    "'{}' has {} in the config but {} in the source",
                    field.name,
                    describe_role(field.role),
                    describe_role(other.role)));
        }
    }
    for (const auto &field : inferred) {
        if (!explicit_names.contains(field.name)) {
            diffs.push_back(std::format("'{}' is only in the source", field.name));
        }
    }

    std::string joined;
    for (const auto &diff : diffs) {
        if (!joined.empty()) {
            joined += "; ";
        }
        joined += diff;
    }
    return joined;
}

// Says what the project's sources failed to state about the declared element width. Each reason names the specific
// thing that is absent or unusable: the width has no second source, so a user who is told only that Porytiles could
// not work it out has nowhere to go looking.
[[nodiscard]] std::string describe_undeclared_width(
    const AttributeDeclarationScan &scan, const std::string &header_source, const TextFormatter *format)
{
    switch (scan.source) {
    case AttributeDeclarationSource::no_fieldmap_header:
        return format->format(
            "This project has no '{}', so nothing in it declares the element type of struct Tileset's "
            "metatileAttributes member.",
            FormatParam{header_source, Style::bold});
    case AttributeDeclarationSource::header_unreadable:
        return format->format(
            "'{}' could not be read, so the element type of struct Tileset's metatileAttributes member is unknown.",
            FormatParam{header_source, Style::bold});
    case AttributeDeclarationSource::no_tileset_struct:
        return format->format(
            "'{}' declares no 'struct Tileset', so nothing declares the element type of its metatileAttributes "
            "member.",
            FormatParam{header_source, Style::bold});
    case AttributeDeclarationSource::no_attributes_member:
        return format->format(
            "'struct Tileset' in '{}' declares no 'metatileAttributes' pointer member.",
            FormatParam{header_source, Style::bold});
    case AttributeDeclarationSource::declared:
        return format->format(
            "'struct Tileset' in '{}' declares '{}', and the engine reads metatile attribute arrays only as u8, u16, "
            "or u32.",
            FormatParam{header_source, Style::bold},
            FormatParam{to_declaration_string(scan), Style::bold});
    }
    panic("unhandled AttributeDeclarationSource value");
}

// Says what inference could not settle about one field of the selected layout, and names the override that settles
// it. Each kind names the specific thing that is absent or contradictory: the fact has no other witness in the
// project, so a user told only that the field could not be resolved has nowhere to go looking.
[[nodiscard]] std::string describe_field_conflict(
    const InferredFieldConflict &conflict, const std::string &header_source, const TextFormatter *format)
{
    switch (conflict.kind) {
    case FieldConflictKind::provider_behaviors_absent:
        return format->format(
            "Metatile attribute field '{}' takes its value names from the behavior constants header, but this "
            "project has no '{}'. Porytiles will not silently fall back to raw numeric values. Restore the header, "
            "or state the field's provider (or 'provider: null' to use raw values deliberately) via "
            "metatile_attribute_field_overrides in your Porytiles config.",
            FormatParam{conflict.field_name, Style::bold},
            FormatParam{conflict.probed, Style::bold});
    case FieldConflictKind::provider_behaviors_unreadable:
        return format->format(
            "Metatile attribute field '{}' takes its value names from the behavior constants header, but '{}' could "
            "not be read, so whatever it declares is unknown. Fix the header so Porytiles can scan it, or state the "
            "field's provider (or 'provider: null' to use raw values deliberately) via "
            "metatile_attribute_field_overrides in your Porytiles config.",
            FormatParam{conflict.field_name, Style::bold},
            FormatParam{conflict.probed, Style::bold});
    case FieldConflictKind::provider_behaviors_no_constants:
        return format->format(
            "Metatile attribute field '{}' takes its value names from the behavior constants header, but '{}' "
            "declares no MB_ name. Porytiles will not silently fall back to raw numeric values. Declare the "
            "constants there, or state the field's provider (or 'provider: null' to use raw values deliberately) "
            "via metatile_attribute_field_overrides in your Porytiles config.",
            FormatParam{conflict.field_name, Style::bold},
            FormatParam{conflict.probed, Style::bold});
    case FieldConflictKind::provider_no_matching_enum:
        return format->format(
            "Metatile attribute field '{}' should take its value names from a '{}' enum, but '{}' declares no enum "
            "member with that prefix. Porytiles will not silently fall back to raw numeric values. Declare the enum, "
            "or state the field's provider (or 'provider: null' to use raw values deliberately) via "
            "metatile_attribute_field_overrides in your Porytiles config.",
            FormatParam{conflict.field_name, Style::bold},
            FormatParam{conflict.probed, Style::bold},
            FormatParam{header_source, Style::bold});
    case FieldConflictKind::mask_define_vs_table:
        return format->format(
            "Metatile attribute field '{}' has mask {} from its METATILE_ATTR define but {} from the "
            "sMetatileAttrMasks table. The project states two different masks for the same field, so it does not in "
            "fact state one, and packing with the wrong mask silently corrupts every attribute word. Make the two "
            "sources agree, or set the field's mask via metatile_attribute_field_overrides in your Porytiles config.",
            FormatParam{conflict.field_name, Style::bold},
            FormatParam{std::format("0x{:X}", conflict.declared.value()), Style::bold},
            FormatParam{std::format("0x{:X}", conflict.alternate.value()), Style::bold});
    case FieldConflictKind::shift_vs_mask:
        return format->format(
            "Metatile attribute field '{}' declares shift {}, but its mask places the field at bit offset {}. The "
            "engine unpacks attribute values with the declared shift while Porytiles packs them at the mask's "
            "offset, so a value written with this layout would not read back as itself. Make the shift match the "
            "mask, or set the field's mask via metatile_attribute_field_overrides in your Porytiles config.",
            FormatParam{conflict.field_name, Style::bold},
            FormatParam{conflict.declared.value()},
            FormatParam{conflict.alternate.value()});
    }
    panic("unhandled FieldConflictKind value");
}

// True when the user's override for the conflicted field speaks to the conflicted fact. The check reads the raw
// overrides rather than the merged result: only the stated override can distinguish "the user chose raw values"
// (provider: null) from "inference never found a provider", which is the difference between a settled fact and a
// guess.
[[nodiscard]] bool
override_settles_conflict(const InferredFieldConflict &conflict, const MetatileAttributeFieldOverrides &overrides)
{
    const auto it = overrides.find(conflict.field_name);
    if (it == overrides.end()) {
        return false;
    }
    switch (conflict.kind) {
    case FieldConflictKind::provider_behaviors_absent:
    case FieldConflictKind::provider_behaviors_unreadable:
    case FieldConflictKind::provider_behaviors_no_constants:
    case FieldConflictKind::provider_no_matching_enum:
        return it->second.provider.has_value();
    case FieldConflictKind::mask_define_vs_table:
    case FieldConflictKind::shift_vs_mask:
        return it->second.mask.has_value();
    }
    panic("unhandled FieldConflictKind value");
}

} // namespace

ChainableResult<LoadedMetatileAttributeSchema> reconcile_metatile_attribute_schema(
    const MetatileAttributeInferenceResult &inference,
    const MetatileAttributeConfigInputs &inputs,
    gsl::not_null<const TextFormatter *> format,
    gsl::not_null<const UserDiagnostics *> diag)
{
    // More than one inferred mask layout (pokeemerald-expansion holds both build flavors) with no explicit width
    // setting is fatal before anything else, even when the fields are explicit. No project file records which flavor
    // the build uses (it is a make argument), the width is the attribute size shared by every tileset, and guessing it
    // wrong silently corrupts the attributes.
    if (inference.candidates.size() >= 2 && !inputs.attribute_size.has_value()) {
        return FormattableError{format->format(
            "Porytiles found more than one metatile attribute mask layout in this project ({}), so it cannot infer "
            "the attribute size. Set 'fieldmap.metatile_attribute_size' in porytiles/config.yaml (or pass "
            "--metatile-attribute-size) to choose the layout this build uses.",
            // Every candidate is listed rather than the first two, matching the selection errors further down: the
            // user needs the full set of widths to know which value to pin.
            FormatParam{describe_candidates(inference.candidates), Style::bold})};
    }

    // Decide the field set. Explicit fields are the truth and inference is never consulted for their content;
    // otherwise the inferred layout must select cleanly or resolution fails with an actionable error.
    MetatileAttributeFieldDefinitions fields = inputs.fields;
    std::string fields_origin;
    const MetatileAttributeCandidateSet *selected = nullptr;
    if (!fields.empty()) {
        fields_origin = std::format("explicit metatile_attribute_fields ({})", inputs.fields_source);
        // Advisory comparison only: when a usable inferred layout exists and disagrees with the explicit fields,
        // warn. Overriding what the project's own source declares is legal but worth knowing about.
        const MetatileAttributeCandidateSet *inferred_layout = nullptr;
        if (inference.status == AttributeInferenceStatus::valid && inference.candidates.size() == 1) {
            inferred_layout = &inference.candidates.front();
        }
        else if (inference.status == AttributeInferenceStatus::valid && inputs.attribute_size.has_value()) {
            const auto matches = match_candidates_to_size(inference.candidates, inputs.attribute_size.value());
            if (matches.exact.size() == 1) {
                inferred_layout = matches.exact.front();
            }
            else if (matches.exact.empty() && matches.narrower.size() == 1) {
                inferred_layout = matches.narrower.front();
            }
        }
        if (inferred_layout != nullptr) {
            const std::string diff = describe_field_diff(fields, inferred_layout->fields);
            if (!diff.empty()) {
                diag->warning(
                    metatile_attr_schema_tag,
                    "The explicit metatile_attribute_fields ({}) do not match the metatile attribute layout "
                    "Porytiles inferred from {}: {}. The explicit fields are used as declared; this warning "
                    "is only a heads-up that they disagree with the project's source.",
                    FormatParam{inputs.fields_source, Style::bold},
                    FormatParam{inferred_layout->origin, Style::bold},
                    FormatParam{diff});
            }
        }
    }
    else if (inference.status == AttributeInferenceStatus::invalid) {
        // The masks are unusable (an undecidable conditional, a declared field with no mask) or could not be read at
        // all, and there are no explicit fields to fall back on, so resolution cannot proceed even when an explicit
        // size pinned the width. Inference already phrased the reason, so it passes through verbatim.
        return FormattableError{inference.error_message};
    }
    else if (inference.candidates.empty()) {
        return FormattableError{std::vector<std::string>{
            "No metatile attribute fields are configured, and Porytiles found no mask layout to infer them from.",
            "It infers the layout from the attribute masks the base game declares: the METATILE_ATTR_*_MASK "
            "defines in 'include/global.fieldmap.h' or the sMetatileAttrMasks table in 'src/fieldmap.c'.",
            "Make sure those masks exist, or add a metatile_attribute_fields list to your Porytiles config."}};
    }
    else if (inference.candidates.size() == 1) {
        selected = &inference.candidates.front();
    }
    else {
        // Two or more candidates; the dual-layout gate above guarantees the width knob is set here. Select by
        // required width: a unique exact match wins; with none, a unique narrower fit wins (the width step below
        // warns that the knob is wider than the layout needs); anything else cannot be decided.
        const std::size_t attribute_size = inputs.attribute_size.value();
        const auto matches = match_candidates_to_size(inference.candidates, attribute_size);
        if (matches.exact.size() == 1) {
            selected = matches.exact.front();
        }
        else if (matches.exact.size() > 1) {
            return FormattableError{format->format(
                "Porytiles inferred more than one metatile attribute mask layout matching the configured "
                "attribute size of {} bytes: {}. It cannot choose between them. Declare "
                "metatile_attribute_fields in your Porytiles config to define the layout explicitly.",
                FormatParam{attribute_size, Style::bold},
                FormatParam{describe_candidates(inference.candidates), Style::bold})};
        }
        else if (matches.narrower.size() == 1) {
            selected = matches.narrower.front();
        }
        else if (matches.narrower.empty()) {
            return FormattableError{format->format(
                "Porytiles inferred these metatile attribute mask layouts from the project: {}. None of them "
                "fits the configured attribute size of {} bytes (from {}). Change "
                "'fieldmap.metatile_attribute_size' (or --metatile-attribute-size) to one of the listed widths, "
                "or declare metatile_attribute_fields in your Porytiles config to define the layout explicitly.",
                FormatParam{describe_candidates(inference.candidates), Style::bold},
                FormatParam{attribute_size, Style::bold},
                FormatParam{inputs.attribute_size_source, Style::bold})};
        }
        else {
            return FormattableError{format->format(
                "Porytiles inferred more than one metatile attribute mask layout that fits within the configured "
                "attribute size of {} bytes: {}. It cannot choose between them. Declare "
                "metatile_attribute_fields in your Porytiles config to define the layout explicitly.",
                FormatParam{attribute_size, Style::bold},
                FormatParam{describe_candidates(inference.candidates), Style::bold})};
        }
    }

    if (selected != nullptr) {
        fields = selected->fields;
        fields_origin = selected->origin;
        diag->remark(
            metatile_attr_schema_tag,
            "Porytiles selected the metatile attribute mask layout inferred from {} ({} bytes).",
            FormatParam{selected->origin, Style::bold},
            FormatParam{selected->required_bytes});
    }

    // Merge the overrides into the baseline fields.
    auto merged_result = merge_field_overrides(fields, inputs.overrides, format);
    if (!merged_result.has_value()) {
        return ChainableResult<LoadedMetatileAttributeSchema>{merged_result};
    }
    MetatileAttributeFieldDefinitions resolved = std::move(merged_result).value();

    // Rule on the selected layout's inference conflicts: each fact inference could not settle is fatal unless the
    // user's override for that field speaks to it (a stated provider, including provider: null, settles a provider
    // hunt; a stated mask settles a mask or shift dispute). The ruling happens here rather than in inference because
    // inference runs before the overrides exist, so a fatal raised there would make the escape hatch unreachable.
    // Explicit metatile_attribute_fields skip this entirely: inference was never consulted for their content, so
    // nothing it failed to settle is in play (selected is null on that path).
    if (selected != nullptr) {
        for (const InferredFieldConflict &conflict : selected->conflicts) {
            if (!override_settles_conflict(conflict, inputs.overrides)) {
                return FormattableError{describe_field_conflict(conflict, inputs.fieldmap_header_source, format)};
            }
        }
    }

    // Resolve the width. The explicit knob is authoritative when set; otherwise the merged masks are the sole
    // evidence, and a wider scanned declaration contradicts them fatally (masks prove a minimum width, never the
    // width itself, and an attribute entry is never narrower than the element type it is stored in).
    const RequiredWidth width = required_width(resolved);
    std::size_t attribute_bytes = 0;
    std::string size_origin;
    if (inputs.attribute_size.has_value()) {
        attribute_bytes = inputs.attribute_size.value();
        size_origin = inputs.attribute_size_source;
        if (width.bytes > attribute_bytes) {
            return FormattableError{format->format(
                "{} needs a {}-byte attribute word, but the metatile attribute size is set to {} bytes (from "
                "{}). That size is the project's read stride, fixed for every tileset, so Porytiles cannot "
                "widen it to fit this mask. Narrow the mask to fit {} bytes, or raise "
                "'fieldmap.metatile_attribute_size' (or --metatile-attribute-size) if the project really uses a "
                "wider attribute word.",
                FormatParam{width.widest_source, Style::bold},
                FormatParam{width.bytes, Style::bold},
                FormatParam{attribute_bytes, Style::bold},
                FormatParam{inputs.attribute_size_source, Style::bold},
                FormatParam{attribute_bytes, Style::bold})};
        }
        // A knob wider than both the merged masks and the scanned declaration width is legal (the unused high bits
        // simply stay zero) but suspicious enough to flag. When the scanned declaration corroborates the knob there
        // is nothing to say.
        if (attribute_bytes > std::max(width.bytes, inference.declaration.size.value_or(0))) {
            diag->warning(
                metatile_attr_schema_tag,
                "The metatile attribute size is set to {} bytes (from {}), which is wider than anything the "
                "project declares: the resolved field masks need only {} bytes. Porytiles will use {}-byte "
                "attributes; the unused high bits stay zero.",
                FormatParam{attribute_bytes, Style::bold},
                FormatParam{inputs.attribute_size_source, Style::bold},
                FormatParam{width.bytes, Style::bold},
                FormatParam{attribute_bytes, Style::bold});
        }
    }
    else {
        attribute_bytes = width.bytes;
        size_origin =
            selected != nullptr
                ? describe_inferred_size_origin(*selected)
                : std::format("derived from the explicit metatile_attribute_fields masks ({})", inputs.fields_source);
        // With no explicit knob, the masks and struct Tileset's declaration must agree exactly. Masks prove a
        // minimum width, never the width itself, so the declaration is the only corroborating witness: a missing
        // one leaves the width resting on masks alone, and a disagreeing one (in either direction; expansion's FRLG
        // build deliberately reads a wider word than it declares) means the project's own sources do not settle
        // which width the build reads.
        if (!inference.declaration.size.has_value()) {
            std::string message = format->format(
                "{} Without an explicit metatile attribute size, that declaration is the only fact that can "
                "corroborate the width the resolved field masks imply, so Porytiles cannot confirm the width this "
                "project reads. Set 'fieldmap.metatile_attribute_size' in porytiles/config.yaml (or pass "
                "--metatile-attribute-size) to state it.",
                FormatParam{
                    describe_undeclared_width(inference.declaration.scan, inputs.fieldmap_header_source, format)});
            if (!inputs.declaration_size.has_value()) {
                // The same missing declaration also leaves the generated array declarations without a width, so the
                // user is told about both knobs in one round trip rather than hitting the second fatal after fixing
                // the first.
                message += " Without a usable declaration, 'fieldmap.metatile_attribute_declaration_size' "
                           "(--metatile-attribute-declaration-size) is also needed for the generated "
                           "gMetatileAttributes_* declarations.";
            }
            return FormattableError{std::move(message)};
        }
        if (inference.declaration.size.value() != attribute_bytes) {
            return FormattableError{format->format(
                "The resolved metatile attribute field masks need {}-byte attributes, but struct Tileset declares "
                "its metatileAttributes member with a {}-byte element type. Masks prove a minimum width, never the "
                "width itself, and a project may legitimately read a different word size than it declares "
                "(pokeemerald-expansion's FRLG build reads 4-byte words from 'const u16' arrays), so Porytiles "
                "cannot tell which width this project reads. Set 'fieldmap.metatile_attribute_size' in "
                "porytiles/config.yaml (or pass --metatile-attribute-size) to pin the width.",
                FormatParam{attribute_bytes, Style::bold},
                FormatParam{inference.declaration.size.value(), Style::bold})};
        }
    }

    // Declaration width: the explicit knob when set, otherwise struct Tileset's declaration. There is no third
    // source. The attribute width above can be derived when the knob is unset because the masks independently bound
    // it, but no mask says anything about the element type the arrays are declared with, and the two widths are
    // genuinely different numbers on real projects (expansion's FRLG build declares 'const u16' arrays that are read
    // as 4-byte words). Defaulting the declaration width to the attribute width would therefore not be a derivation,
    // it would be a guess, and the value it guesses is written straight into the project's C headers.
    std::optional<std::size_t> declaration_size = inputs.declaration_size;
    std::string declaration_origin = "explicit metatile_attribute_declaration_size";
    if (!declaration_size.has_value()) {
        if (!inference.declaration.size.has_value()) {
            return FormattableError{format->format(
                "{} Porytiles needs that width to declare the generated gMetatileAttributes_* arrays, and nothing "
                "else in the project implies it. Set 'fieldmap.metatile_attribute_declaration_size' in "
                "porytiles/config.yaml (or pass --metatile-attribute-declaration-size) to state it.",
                FormatParam{
                    describe_undeclared_width(inference.declaration.scan, inputs.fieldmap_header_source, format)})};
        }
        declaration_size = inference.declaration.size;
        declaration_origin =
            std::format("inferred from struct Tileset's metatileAttributes member ({})", inputs.fieldmap_header_source);
    }
    const std::size_t declaration_bytes = declaration_size.value();

    auto schema_result = build_schema(resolved, attribute_bytes);
    if (!schema_result.has_value()) {
        return ChainableResult<LoadedMetatileAttributeSchema>{schema_result};
    }

    LoadedMetatileAttributeSchema loaded{
        std::move(schema_result).value(),
        std::move(resolved),
        attribute_bytes,
        declaration_bytes,
        std::move(fields_origin),
        std::move(size_origin),
        std::move(declaration_origin)};

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
    diag->remark(
        metatile_attr_schema_tag,
        "Porytiles resolved {}-byte metatile attributes with fields: {} ({}).",
        FormatParam{loaded.attribute_bytes, Style::bold},
        FormatParam{field_names, Style::bold},
        FormatParam{layer_type_note, Style::bold});

    return loaded;
}

} // namespace porytiles
