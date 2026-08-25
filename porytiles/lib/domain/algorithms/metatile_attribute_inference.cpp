#include "porytiles/domain/algorithms/metatile_attribute_inference.hpp"

#include <algorithm>
#include <bit>
#include <cctype>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/utilities/string_utils.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

namespace {

constexpr const char *attribute_enum_prefix = "METATILE_ATTRIBUTE_";
constexpr const char *mask_define_prefix = "METATILE_ATTR_";
constexpr const char *mask_define_suffix = "_MASK";
constexpr const char *frlg_mask_define_suffix = "_MASK_FRLG";
constexpr const char *shift_define_suffix = "_SHIFT";
constexpr const char *frlg_shift_define_suffix = "_SHIFT_FRLG";
constexpr const char *behaviors_header = "include/constants/metatile_behaviors.h";
constexpr const char *fieldmap_header = "include/global.fieldmap.h";

[[nodiscard]] std::string to_lower(std::string text)
{
    std::ranges::transform(text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

// A base game may spell the layer field's enum member LAYER or LAYER_TYPE; both mean the same field.
[[nodiscard]] std::string normalize_suffix(std::string suffix)
{
    if (suffix == "LAYER") {
        return "LAYER_TYPE";
    }
    return suffix;
}

[[nodiscard]] bool is_all_digits(const std::string &text)
{
    return !text.empty() && std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
}

[[nodiscard]] bool any_enum_member_has_prefix(const MetatileAttributeScan &scan, const std::string &prefix)
{
    return std::any_of(scan.enum_members.begin(), scan.enum_members.end(), [&](const InferenceEnumMember &member) {
        return member.name.starts_with(prefix);
    });
}

// Joins the file paths that fed one candidate set, skipping any the scan did not record. A set names only the files
// its origin prose names, so the two stay in agreement.
[[nodiscard]] std::string join_sources(const std::string &first, const std::string &second)
{
    if (first.empty()) {
        return second;
    }
    if (second.empty()) {
        return first;
    }
    return first + ", " + second;
}

// Per-suffix mask and shift facts collected in Phase A before the sets are grouped.
struct SuffixMasks {
    std::optional<std::uint32_t> bare_define;
    std::optional<std::uint32_t> frlg_define;
    std::optional<std::uint32_t> array_value;
    std::optional<std::uint32_t> bare_shift_define;
    std::optional<std::uint32_t> frlg_shift_define;
};

// Phase B naming: the display name, optional value-name provider, and optional provider conflict for one suffix.
// Layout-independent, so it is computed once per suffix and shared by every candidate set that includes the field.
// A conflict is recorded rather than diagnosed: inference runs before the user's overrides are merged, so whether a
// missing provider is fatal is the reconciler's call, not this function's.
struct FieldNaming {
    std::string name;
    std::optional<ProviderDefinition> provider;
    std::optional<InferredFieldConflict> conflict;
};

[[nodiscard]] FieldNaming name_field(const std::string &suffix, const MetatileAttributeScan &scan)
{
    FieldNaming naming;

    if (suffix == "LAYER_TYPE") {
        // The layer_type field's values are managed by Porytiles (FieldRole::layer_type), so its name is hardcoded
        // rather than derived and it never gets a value provider.
        naming.name = std::string{attribute::field_layer_type};
        return naming;
    }

    if (suffix == "BEHAVIOR") {
        naming.name = "behavior";
        // Diagnostics name the path the scan actually looked at; the canonical relative path stands in when the scan
        // recorded none (a unit-test scan, or a scan that never reached the behaviors header).
        const std::string behaviors_path =
            scan.behaviors_header.path.empty() ? std::string{behaviors_header} : scan.behaviors_header.path;
        switch (scan.behaviors_header.source) {
        case BehaviorsHeaderSource::declared:
            naming.provider = ProviderDefinition{behaviors_header, "MB_", {"MB_INVALID"}, HeaderFormat::either};
            break;
        case BehaviorsHeaderSource::absent:
            naming.conflict = InferredFieldConflict{
                naming.name, FieldConflictKind::provider_behaviors_absent, behaviors_path, std::nullopt, std::nullopt};
            break;
        case BehaviorsHeaderSource::unreadable:
            naming.conflict = InferredFieldConflict{
                naming.name,
                FieldConflictKind::provider_behaviors_unreadable,
                behaviors_path,
                std::nullopt,
                std::nullopt};
            break;
        case BehaviorsHeaderSource::no_constants:
            naming.conflict = InferredFieldConflict{
                naming.name,
                FieldConflictKind::provider_behaviors_no_constants,
                behaviors_path,
                std::nullopt,
                std::nullopt};
            break;
        }
    }
    else if (is_all_digits(suffix)) {
        // A purely numeric suffix (METATILE_ATTRIBUTE_2) is the project stating the field has no name and no
        // constants. Nothing was inferred, so nothing failed: no provider and no conflict.
        naming.name = "attribute_" + suffix;
    }
    else {
        naming.name = to_lower(suffix);
        std::string probe = "TILE_" + suffix + "_";
        if (!any_enum_member_has_prefix(scan, probe) && suffix.ends_with("_TYPE")) {
            probe = "TILE_" + suffix.substr(0, suffix.size() - std::string_view{"_TYPE"}.size()) + "_";
        }
        if (any_enum_member_has_prefix(scan, probe)) {
            naming.provider = ProviderDefinition{fieldmap_header, probe, {}, HeaderFormat::enums_only};
        }
        else {
            naming.conflict = InferredFieldConflict{
                naming.name, FieldConflictKind::provider_no_matching_enum, probe, std::nullopt, std::nullopt};
        }
    }

    return naming;
}

// Maps struct Tileset's metatileAttributes declarator to a declared element width. Every base game declares the member
// as 'const uN *metatileAttributes' (u16 on pokeemerald and expansion, u32 on pokefirered). A declarator that is not a
// single pointer to u8, u16, or u32 names no width the engine can read, so the width stays unset and reconciliation
// rules on what to do about it. The scan travels along either way, so the ruling can quote the declaration.
[[nodiscard]] InferredAttributeDeclaration interpret_declaration(const AttributeDeclarationScan &scan)
{
    InferredAttributeDeclaration declaration;
    declaration.scan = scan;
    if (scan.source != AttributeDeclarationSource::declared || scan.pointer_depth != 1) {
        return declaration;
    }
    if (scan.element_type == "u8") {
        declaration.size = 1;
    }
    else if (scan.element_type == "u16") {
        declaration.size = 2;
    }
    else if (scan.element_type == "u32") {
        declaration.size = 4;
    }
    return declaration;
}

// The smallest of 1, 2, or 4 bytes that covers every mask in a candidate set. The layer-type field is an
// ordinary member of the set, so its mask participates like any other.
[[nodiscard]] std::size_t required_bytes_for(const MetatileAttributeCandidateSet &candidate)
{
    std::size_t required_bits = 0;
    for (const auto &field : candidate.fields) {
        required_bits = std::max(required_bits, static_cast<std::size_t>(std::bit_width(field.mask.value())));
    }
    return required_bits <= 8 ? 1U : (required_bits <= 16 ? 2U : 4U);
}

} // namespace

std::string to_declaration_string(const AttributeDeclarationScan &scan)
{
    if (scan.source != AttributeDeclarationSource::declared) {
        return {};
    }
    std::string text;
    if (scan.is_const) {
        text += "const ";
    }
    text += scan.element_type;
    text += ' ';
    text.append(scan.pointer_depth, '*');
    text += "metatileAttributes";
    return text;
}

MetatileAttributeInferenceResult
infer_metatile_attribute_candidates(const MetatileAttributeScan &scan, gsl::not_null<const TextFormatter *> format)
{
    MetatileAttributeInferenceResult result;

    // The declaration width is a project fact independent of the mask layout, so it is mapped up front and survives
    // every status, including the early invalid returns below.
    result.declaration = interpret_declaration(scan.declaration);

    // --- Phase A: gather suffixes and masks ---

    for (const std::string &name : scan.ambiguous_defines) {
        if (!name.starts_with(mask_define_prefix)) {
            continue;
        }
        const std::string_view body = std::string_view{name}.substr(std::string_view{mask_define_prefix}.size());
        if (!body.ends_with(mask_define_suffix) && !body.ends_with(frlg_mask_define_suffix)) {
            continue;
        }
        result.status = AttributeInferenceStatus::invalid;
        result.error_message = format->format(
            "Metatile attribute mask define '{}' has conflicting values in a conditional Porytiles could not "
            "evaluate. Porytiles cannot infer a safe attribute layout from that definition. Set the mask explicitly "
            "with metatile_attribute_field_overrides or declare metatile_attribute_fields in your Porytiles config.",
            FormatParam{name, Style::bold});
        return result;
    }

    // Ordered, de-duplicated suffix list: enum suffixes first (declaration order), then define-only suffixes.
    std::vector<std::string> ordered_suffixes;
    std::unordered_set<std::string> seen_suffixes;
    const auto add_suffix = [&](const std::string &suffix) {
        if (seen_suffixes.insert(suffix).second) {
            ordered_suffixes.push_back(suffix);
        }
    };

    for (const auto &member : scan.enum_members) {
        if (!member.name.starts_with(attribute_enum_prefix)) {
            continue;
        }
        std::string suffix = normalize_suffix(member.name.substr(std::string_view{attribute_enum_prefix}.size()));
        if (suffix == "COUNT") {
            continue; // the count sentinel is not a field
        }
        add_suffix(suffix);
    }

    std::unordered_map<std::string, SuffixMasks> masks;
    bool any_bare_define = false;
    bool any_frlg_define = false;
    for (const auto &define : scan.defines) {
        if (!define.name.starts_with(mask_define_prefix)) {
            continue;
        }
        const std::string body = define.name.substr(std::string_view{mask_define_prefix}.size());
        std::string suffix;
        bool is_frlg = false;
        bool is_shift = false;
        if (body.ends_with(frlg_mask_define_suffix)) {
            suffix = body.substr(0, body.size() - std::string_view{frlg_mask_define_suffix}.size());
            is_frlg = true;
        }
        else if (body.ends_with(mask_define_suffix)) {
            suffix = body.substr(0, body.size() - std::string_view{mask_define_suffix}.size());
        }
        else if (body.ends_with(frlg_shift_define_suffix)) {
            suffix = body.substr(0, body.size() - std::string_view{frlg_shift_define_suffix}.size());
            is_frlg = true;
            is_shift = true;
        }
        else if (body.ends_with(shift_define_suffix)) {
            suffix = body.substr(0, body.size() - std::string_view{shift_define_suffix}.size());
            is_shift = true;
        }
        else {
            // A define carrying the attribute prefix with a suffix none of the four branches above read. Recorded so
            // the caller can say it was ignored: a user who spells a mask METATILE_ATTR_BEHAVIOR_MASK_RSE gets a
            // layout built without it, and dropping it in silence reads as Porytiles having accepted it.
            result.unrecognized_defines.push_back(define.name);
            continue;
        }
        if (suffix.empty()) {
            continue;
        }
        suffix = normalize_suffix(suffix);
        if (is_shift) {
            // A shift define is a cross-check on its mask, never a field source: it is recorded for the conflict
            // checks below but does not introduce a suffix.
            if (is_frlg) {
                masks[suffix].frlg_shift_define = define.value;
            }
            else {
                masks[suffix].bare_shift_define = define.value;
            }
            continue;
        }
        if (is_frlg) {
            masks[suffix].frlg_define = define.value;
            any_frlg_define = true;
        }
        else {
            masks[suffix].bare_define = define.value;
            any_bare_define = true;
        }
        add_suffix(suffix);
    }

    bool any_array_value = false;
    for (const auto &entry : scan.masks_array) {
        if (!entry.index_name.starts_with(attribute_enum_prefix)) {
            continue;
        }
        std::string suffix = normalize_suffix(entry.index_name.substr(std::string_view{attribute_enum_prefix}.size()));
        if (suffix == "COUNT") {
            continue;
        }
        if (entry.value.has_value()) {
            masks[suffix].array_value = entry.value;
            any_array_value = true;
        }
        // Array entries only fill in suffixes already known from the enum; they do not introduce new fields.
    }

    std::unordered_map<std::string, std::uint32_t> shifts;
    for (const auto &entry : scan.shifts_array) {
        if (!entry.index_name.starts_with(attribute_enum_prefix) || !entry.value.has_value()) {
            continue;
        }
        std::string suffix = normalize_suffix(entry.index_name.substr(std::string_view{attribute_enum_prefix}.size()));
        shifts[suffix] = entry.value.value();
    }

    // A file the scan could not read is the likely reason nothing usable turned up, so it is reported as such rather
    // than as "this project declares no masks": the masks may well be sitting in the file, unread. Both no-candidate
    // exits below route through here. The message stands on its own rather than deferring to the scanner's warning
    // about the same file, since that warning is subject to the user's diagnostic filters and this is not.
    const auto no_usable_layout = [&]() -> MetatileAttributeInferenceResult & {
        if (scan.unreadable_sources.empty()) {
            result.status = AttributeInferenceStatus::not_provided;
            return result;
        }
        result.status = AttributeInferenceStatus::invalid;
        result.error_message = format->format(
            "Porytiles could not read {}, so it has no metatile attribute masks to infer a layout from, and no "
            "metatile_attribute_fields list is configured to stand in for them. Fix those files so Porytiles can "
            "scan them, or declare the layout explicitly with metatile_attribute_fields in your Porytiles config.",
            // The list is short (at most the fieldmap header, its source file, and the behaviors header), so every
            // path is named rather than summarized.
            FormatParam{join_quoted(scan.unreadable_sources), Style::bold});
        return result;
    };

    if (ordered_suffixes.empty()) {
        // Nothing attribute-related was found anywhere; defer to other providers.
        return no_usable_layout();
    }

    // --- Phase B: name each suffix and attach providers (layout-independent, shared by all sets) ---

    std::unordered_map<std::string, FieldNaming> namings;
    for (const auto &suffix : ordered_suffixes) {
        namings.emplace(suffix, name_field(suffix, scan));
    }

    // --- Phase C: group masks into candidate sets, rejecting fields with no mask in any set ---

    // Describes one candidate set being assembled: which per-suffix mask slot feeds it, what to call it, and which
    // files it was read from. The masks live in two different files (the header defines and the src/fieldmap.c table),
    // so the paths are decided here, next to the prose naming those same sources.
    struct SetPlan {
        std::string origin;
        std::string source;
        bool frlg; // true selects frlg_define ?? array_value, false selects the bare define (?? array when single)
    };
    std::vector<SetPlan> plans;
    if (any_frlg_define) {
        // Dual layout: bare defines are one set; FRLG defines (plus the table, which describes the FRLG layout in
        // these projects) are the other.
        plans.push_back(SetPlan{"the bare METATILE_ATTR_*_MASK defines", scan.header_source, false});
        if (any_array_value) {
            plans.push_back(
                SetPlan{
                    "the METATILE_ATTR_*_MASK_FRLG defines and the sMetatileAttrMasks table",
                    join_sources(scan.header_source, scan.masks_table_source),
                    true});
        }
        else {
            plans.push_back(SetPlan{"the METATILE_ATTR_*_MASK_FRLG defines", scan.header_source, true});
        }
    }
    else {
        std::string origin;
        std::string source;
        if (any_bare_define && any_array_value) {
            origin = "the METATILE_ATTR_*_MASK defines and the sMetatileAttrMasks table";
            source = join_sources(scan.header_source, scan.masks_table_source);
        }
        else if (any_bare_define) {
            origin = "the METATILE_ATTR_*_MASK defines";
            source = scan.header_source;
        }
        else {
            origin = "the sMetatileAttrMasks table";
            source = scan.masks_table_source;
        }
        plans.push_back(SetPlan{std::move(origin), std::move(source), false});
    }

    // Resolve the mask one set selects for one suffix. In a dual-layout project the bare define alone is the primary
    // mask; in a single-layout project the define wins over a disagreeing table entry.
    const auto mask_for = [&](const SuffixMasks &m, bool frlg) -> std::optional<std::uint32_t> {
        if (frlg) {
            return m.frlg_define.has_value() ? m.frlg_define : m.array_value;
        }
        if (any_frlg_define) {
            return m.bare_define;
        }
        return m.bare_define.has_value() ? m.bare_define : m.array_value;
    };

    // Records the facts inference could not settle for one field of one set. The pairing rules mirror how the masks
    // themselves are resolved: in a dual layout the mask/shift tables and the _FRLG defines describe the FRLG set
    // while the bare defines describe the bare set; in a single layout everything pairs with the one merged set.
    const auto collect_conflicts = [&](const std::string &suffix,
                                       const SuffixMasks &m,
                                       std::uint32_t resolved_mask,
                                       bool frlg,
                                       std::vector<InferredFieldConflict> &conflicts) {
        const FieldNaming &naming = namings.at(suffix);
        if (naming.conflict.has_value()) {
            conflicts.push_back(naming.conflict.value());
        }

        // A mask define disagreeing with the mask table: the project states two different masks for the same field,
        // so it does not in fact state one. Only the set the table feeds can carry this conflict.
        const auto &paired_define = frlg ? m.frlg_define : m.bare_define;
        if (frlg == any_frlg_define && paired_define.has_value() && m.array_value.has_value() &&
            paired_define.value() != m.array_value.value()) {
            conflicts.push_back(
                InferredFieldConflict{
                    naming.name, FieldConflictKind::mask_define_vs_table, {}, paired_define, m.array_value});
        }

        // Declared shifts must equal the resolved mask's low-bit offset: the engine unpacks with the shift while
        // Porytiles packs at the mask offset, so a disagreement means every written value reads back wrong.
        const auto expected = static_cast<std::uint32_t>(std::countr_zero(resolved_mask));
        const auto &shift_define = frlg ? m.frlg_shift_define : m.bare_shift_define;
        if (shift_define.has_value() && shift_define.value() != expected) {
            conflicts.push_back(
                InferredFieldConflict{naming.name, FieldConflictKind::shift_vs_mask, {}, shift_define, expected});
        }
        // The shift table pairs with whichever set the mask table feeds (the FRLG set in a dual layout).
        if (frlg == any_frlg_define) {
            if (const auto shift_it = shifts.find(suffix); shift_it != shifts.end() && shift_it->second != expected) {
                conflicts.push_back(
                    InferredFieldConflict{
                        naming.name, FieldConflictKind::shift_vs_mask, {}, shift_it->second, expected});
            }
        }
    };

    // Assemble the sets. A suffix missing a mask in one set is simply excluded from that set, but a suffix missing a
    // mask in every set means the project declares a field it exposes no mask for, which is fatal. The layer_type
    // field is covered like any other; its suggested define spelling follows the emerald family's
    // METATILE_ATTR_LAYER_MASK rather than the normalized LAYER_TYPE suffix.
    for (const auto &suffix : ordered_suffixes) {
        const auto it = masks.find(suffix);
        const SuffixMasks m = (it != masks.end()) ? it->second : SuffixMasks{};
        const bool covered = std::any_of(
            plans.begin(), plans.end(), [&](const SetPlan &plan) { return mask_for(m, plan.frlg).has_value(); });
        if (!covered) {
            const std::string define_stem = suffix == "LAYER_TYPE" ? "LAYER" : suffix;
            result.status = AttributeInferenceStatus::invalid;
            result.error_message = format->format(
                "Could not determine a bit mask for metatile attribute field '{}'. The base game declares this "
                "field but exposes no mask for it. Provide one of: restore the sMetatileAttrMasks[] table under its "
                "exact name in src/fieldmap.c; add a METATILE_ATTR_{}_MASK #define in {}; or set the mask "
                "explicitly via metatile_attribute_field_overrides (or a full metatile_attribute_fields list) in "
                "your Porytiles config.",
                FormatParam{namings.at(suffix).name, Style::bold},
                FormatParam{define_stem},
                FormatParam{fieldmap_header, Style::bold});
            return result;
        }
    }

    for (const auto &plan : plans) {
        MetatileAttributeCandidateSet candidate;
        candidate.origin = plan.origin;
        candidate.source = plan.source;
        bool any_value_field = false;
        for (const auto &suffix : ordered_suffixes) {
            const auto it = masks.find(suffix);
            const SuffixMasks m = (it != masks.end()) ? it->second : SuffixMasks{};
            const auto mask = mask_for(m, plan.frlg);
            if (!mask.has_value()) {
                continue; // excluded from this set; another set covers it
            }
            collect_conflicts(suffix, m, mask.value(), plan.frlg, candidate.conflicts);
            const FieldNaming &naming = namings.at(suffix);
            MetatileAttributeFieldDefinition definition;
            definition.name = naming.name;
            definition.mask = mask;
            definition.provider = naming.provider;
            if (suffix == "LAYER_TYPE") {
                definition.role = FieldRole::layer_type;
            }
            else {
                any_value_field = true;
            }
            candidate.fields.push_back(std::move(definition));
        }
        if (!any_value_field) {
            continue; // a set with nothing beyond the layer-type role field is not a usable layout
        }
        candidate.required_bytes = required_bytes_for(candidate);
        result.candidates.push_back(std::move(candidate));
    }

    if (result.candidates.empty()) {
        // Every discovered suffix was the managed layer_type; nothing usable to provide.
        return no_usable_layout();
    }

    result.status = AttributeInferenceStatus::valid;
    return result;
}

} // namespace porytiles
