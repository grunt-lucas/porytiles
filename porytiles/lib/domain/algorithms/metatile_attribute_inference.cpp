#include "porytiles/domain/algorithms/metatile_attribute_inference.hpp"

#include <algorithm>
#include <bit>
#include <cctype>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

namespace {

constexpr const char *attribute_enum_prefix = "METATILE_ATTRIBUTE_";
constexpr const char *mask_define_prefix = "METATILE_ATTR_";
constexpr const char *mask_define_suffix = "_MASK";
constexpr const char *frlg_mask_define_suffix = "_MASK_FRLG";
constexpr const char *behaviors_header = "include/constants/metatile_behaviors.h";
constexpr const char *fieldmap_header = "include/global.fieldmap.h";

[[nodiscard]] std::string to_lower(std::string text)
{
    std::transform(
        text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
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

// Per-suffix mask facts collected in Phase A before the sets are grouped.
struct SuffixMasks {
    std::optional<std::uint32_t> bare_define;
    std::optional<std::uint32_t> frlg_define;
    std::optional<std::uint32_t> array_value;
};

// Phase B naming: the display name and optional value-name provider for one suffix. Layout-independent, so it is
// computed once per suffix and shared by every candidate set that includes the field.
struct FieldNaming {
    std::string name;
    std::optional<ProviderSpec> provider;
};

[[nodiscard]] FieldNaming name_field(
    const std::string &suffix,
    const MetatileAttributeScan &scan,
    gsl::not_null<const TextFormatter *> format,
    std::vector<std::string> &warnings)
{
    FieldNaming naming;

    if (suffix == "BEHAVIOR") {
        naming.name = "behavior";
        if (scan.behaviors_header_present) {
            naming.provider = ProviderSpec{behaviors_header, "MB_", {"MB_INVALID"}, HeaderFormat::either};
        }
        else {
            warnings.push_back(format->format(
                "no behavior constants found in {}; leaving the behavior field without a value provider",
                FormatParam{behaviors_header, Style::bold}));
        }
    }
    else if (is_all_digits(suffix)) {
        naming.name = "attribute_" + suffix;
    }
    else {
        naming.name = to_lower(suffix);
        std::string probe = "TILE_" + suffix + "_";
        if (!any_enum_member_has_prefix(scan, probe) && suffix.ends_with("_TYPE")) {
            probe = "TILE_" + suffix.substr(0, suffix.size() - std::string_view{"_TYPE"}.size()) + "_";
        }
        if (any_enum_member_has_prefix(scan, probe)) {
            naming.provider = ProviderSpec{fieldmap_header, probe, {}, HeaderFormat::enums_only};
        }
    }

    return naming;
}

// Maps the pointed-to type of struct Tileset's metatileAttributes member to a declared element width. Every base
// game declares the member as 'const uN *metatileAttributes' (u16 on pokeemerald and expansion, u32 on pokefirered);
// anything else (missing declaration, unknown type) yields nullopt so the downstream default ("match the attribute
// size") applies.
[[nodiscard]] std::optional<std::size_t> declaration_size_from(const std::optional<std::string> &element_type)
{
    if (!element_type.has_value()) {
        return std::nullopt;
    }
    if (element_type.value() == "u8") {
        return 1;
    }
    if (element_type.value() == "u16") {
        return 2;
    }
    if (element_type.value() == "u32") {
        return 4;
    }
    return std::nullopt;
}

// The smallest of 1, 2, or 4 bytes that covers every mask in a candidate set (fields and layer type).
[[nodiscard]] std::size_t required_bytes_for(const MetatileAttributeCandidateSet &candidate)
{
    std::size_t required_bits = 0;
    for (const auto &field : candidate.fields) {
        required_bits = std::max(required_bits, static_cast<std::size_t>(std::bit_width(field.mask.value())));
    }
    if (candidate.layer_type_mask.has_value()) {
        required_bits =
            std::max(required_bits, static_cast<std::size_t>(std::bit_width(candidate.layer_type_mask.value())));
    }
    return required_bits <= 8 ? 1U : (required_bits <= 16 ? 2U : 4U);
}

} // namespace

MetatileAttributeInferenceResult
infer_metatile_attribute_candidates(const MetatileAttributeScan &scan, gsl::not_null<const TextFormatter *> format)
{
    MetatileAttributeInferenceResult result;

    // The declaration width is a project fact independent of the mask layout, so it is mapped up front and survives
    // every status, including the early invalid returns below.
    result.declaration_size = declaration_size_from(scan.attributes_element_type);

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
        if (body.ends_with(frlg_mask_define_suffix)) {
            suffix = body.substr(0, body.size() - std::string_view{frlg_mask_define_suffix}.size());
            is_frlg = true;
        }
        else if (body.ends_with(mask_define_suffix)) {
            suffix = body.substr(0, body.size() - std::string_view{mask_define_suffix}.size());
        }
        else {
            continue;
        }
        if (suffix.empty()) {
            continue;
        }
        suffix = normalize_suffix(suffix);
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

    if (ordered_suffixes.empty()) {
        // Nothing attribute-related was found anywhere; defer to other providers.
        result.status = AttributeInferenceStatus::not_provided;
        return result;
    }

    // --- Phase B: name each suffix and attach providers (layout-independent, shared by all sets) ---

    std::unordered_map<std::string, FieldNaming> namings;
    for (const auto &suffix : ordered_suffixes) {
        if (suffix == "LAYER_TYPE") {
            continue; // structural, never emitted as a field
        }
        namings.emplace(suffix, name_field(suffix, scan, format, result.warnings));
    }

    // --- Phase C: group masks into candidate sets, filling or rejecting fields with no mask ---

    // The stock two-byte layout exception: a project declaring exactly BEHAVIOR and LAYER_TYPE with no masks anywhere
    // is completed as one synthesized candidate (behavior in the low byte, two-byte word). The gate is structural
    // rather than size-based: with no masks there is nothing to infer a size from, and every real stock project that
    // hits this shape is the two-byte emerald family.
    const bool behavior_only_stock = ordered_suffixes.size() == 2 && seen_suffixes.contains("BEHAVIOR") &&
                                     seen_suffixes.contains("LAYER_TYPE") && masks.empty();
    if (behavior_only_stock) {
        MetatileAttributeCandidateSet candidate;
        candidate.origin = "the stock two-byte behavior-only layout (assumed)";
        MetatileAttributeFieldSpec spec;
        spec.name = namings.at("BEHAVIOR").name;
        spec.mask = 0x00FFU; // stock two-byte layout: behavior occupies the low byte
        spec.provider = namings.at("BEHAVIOR").provider;
        candidate.fields.push_back(std::move(spec));
        candidate.required_bytes = 2;
        candidate.synthesized = true;
        result.candidates.push_back(std::move(candidate));
        result.status = AttributeInferenceStatus::valid;
        return result;
    }

    // Describes one candidate set being assembled: which per-suffix mask slot feeds it and what to call it.
    struct SetPlan {
        std::string origin;
        bool frlg; // true selects frlg_define ?? array_value, false selects the bare define (?? array when single)
    };
    std::vector<SetPlan> plans;
    if (any_frlg_define) {
        // Dual layout: bare defines are one set; FRLG defines (plus the table, which describes the FRLG layout in
        // these projects) are the other.
        plans.push_back(SetPlan{"the bare METATILE_ATTR_*_MASK defines", false});
        plans.push_back(SetPlan{"the METATILE_ATTR_*_MASK_FRLG defines and the sMetatileAttrMasks table", true});
    }
    else {
        std::string origin;
        if (any_bare_define && any_array_value) {
            origin = "the METATILE_ATTR_*_MASK defines and the sMetatileAttrMasks table";
        }
        else if (any_bare_define) {
            origin = "the METATILE_ATTR_*_MASK defines";
        }
        else {
            origin = "the sMetatileAttrMasks table";
        }
        plans.push_back(SetPlan{std::move(origin), false});
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

    // Emit the mask-conflict warnings once (not per set): in a dual layout the table pairs with the FRLG defines, in
    // a single layout it pairs with the bare defines.
    for (const auto &suffix : ordered_suffixes) {
        const auto it = masks.find(suffix);
        if (it == masks.end()) {
            continue;
        }
        const SuffixMasks &m = it->second;
        const std::string display_name = suffix == "LAYER_TYPE" ? "layer_type" : namings.at(suffix).name;
        if (any_frlg_define) {
            if (m.frlg_define.has_value() && m.array_value.has_value() &&
                m.frlg_define.value() != m.array_value.value()) {
                result.warnings.push_back(format->format(
                    "field '{}' FRLG mask define disagrees with the mask table; using the define",
                    FormatParam{display_name, Style::bold}));
            }
        }
        else if (
            m.bare_define.has_value() && m.array_value.has_value() && m.bare_define.value() != m.array_value.value()) {
            result.warnings.push_back(format->format(
                "field '{}' mask define disagrees with the mask table; using the define",
                FormatParam{display_name, Style::bold}));
        }
    }

    // Shift table cross-check: the recorded shift should equal the mask's low-bit offset. The
    // sMetatileAttrMasks/sMetatileAttrShifts tables describe the FRLG layout in a dual-layout project, so the shift
    // is checked against that set's mask; a single-layout project checks the merged mask.
    for (const auto &[suffix, shift] : shifts) {
        if (!seen_suffixes.contains(suffix)) {
            continue; // table-only suffixes never become fields, so their shifts are not cross-checked
        }
        const auto it = masks.find(suffix);
        if (it == masks.end()) {
            continue;
        }
        const auto mask_for_check = mask_for(it->second, any_frlg_define);
        if (!mask_for_check.has_value()) {
            continue;
        }
        const auto expected = static_cast<std::uint32_t>(std::countr_zero(mask_for_check.value()));
        if (shift != expected) {
            const std::string display_name = suffix == "LAYER_TYPE" ? "layer_type" : namings.at(suffix).name;
            result.warnings.push_back(format->format(
                "field '{}' shift table entry ({}) does not match its mask offset ({}); using the mask",
                FormatParam{display_name, Style::bold},
                FormatParam{shift},
                FormatParam{expected}));
        }
    }

    // Assemble the sets. A suffix missing a mask in one set is simply excluded from that set, but a suffix missing a
    // mask in every set means the project declares a field it exposes no mask for, which is fatal.
    for (const auto &suffix : ordered_suffixes) {
        if (suffix == "LAYER_TYPE") {
            continue; // handled below, per set
        }
        const auto it = masks.find(suffix);
        const SuffixMasks m = (it != masks.end()) ? it->second : SuffixMasks{};
        const bool covered = std::any_of(
            plans.begin(), plans.end(), [&](const SetPlan &plan) { return mask_for(m, plan.frlg).has_value(); });
        if (!covered) {
            result.status = AttributeInferenceStatus::invalid;
            result.error_message = format->format(
                "could not determine a bit mask for metatile attribute field '{}'. The base game declares this "
                "field but exposes no mask for it. Provide one of: restore the sMetatileAttrMasks[] table under its "
                "exact name in src/fieldmap.c; add a METATILE_ATTR_{}_MASK #define in {}; or set the mask "
                "explicitly via metatile_attribute_field_overrides (or a full metatile_attribute_fields list) in "
                "your Porytiles config.",
                FormatParam{namings.at(suffix).name, Style::bold},
                FormatParam{suffix},
                FormatParam{fieldmap_header, Style::bold});
            return result;
        }
    }

    for (const auto &plan : plans) {
        MetatileAttributeCandidateSet candidate;
        candidate.origin = plan.origin;
        for (const auto &suffix : ordered_suffixes) {
            const auto it = masks.find(suffix);
            const SuffixMasks m = (it != masks.end()) ? it->second : SuffixMasks{};
            const auto mask = mask_for(m, plan.frlg);
            if (suffix == "LAYER_TYPE") {
                candidate.layer_type_mask = mask;
                continue;
            }
            if (!mask.has_value()) {
                continue; // excluded from this set; another set covers it
            }
            const FieldNaming &naming = namings.at(suffix);
            MetatileAttributeFieldSpec spec;
            spec.name = naming.name;
            spec.mask = mask;
            spec.provider = naming.provider;
            candidate.fields.push_back(std::move(spec));
        }
        if (candidate.fields.empty()) {
            continue; // a set with no fields (e.g. only a layer mask on one side) is not a usable layout
        }
        candidate.required_bytes = required_bytes_for(candidate);
        result.candidates.push_back(std::move(candidate));
    }

    if (result.candidates.empty()) {
        // Every discovered suffix was structural (e.g. only LAYER_TYPE); nothing usable to provide.
        result.status = AttributeInferenceStatus::not_provided;
        return result;
    }

    result.status = AttributeInferenceStatus::valid;
    return result;
}

} // namespace porytiles
