#include "porytiles/infra/config/metatile_attribute_inference.hpp"

#include <algorithm>
#include <bit>
#include <cctype>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

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

// Per-suffix mask facts collected in Phase A before layout rules are applied.
struct SuffixMasks {
    std::optional<std::uint32_t> bare_define;
    std::optional<std::uint32_t> frlg_define;
    std::optional<std::uint32_t> array_value;
};

// The resolved per-field masks after applying the single/dual-layout rule.
struct FieldMasks {
    std::string suffix;
    std::optional<std::uint32_t> primary;
    std::optional<std::uint32_t> frlg;
};

} // namespace

MetatileAttributeInferenceResult
infer_metatile_attribute_fields(const MetatileAttributeScan &scan, gsl::not_null<const TextFormatter *> format)
{
    MetatileAttributeInferenceResult result;

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
        }
        add_suffix(suffix);
    }

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

    // Resolve primary and FRLG masks per suffix according to the layout rule.
    std::vector<FieldMasks> field_masks;
    field_masks.reserve(ordered_suffixes.size());
    for (const auto &suffix : ordered_suffixes) {
        const auto it = masks.find(suffix);
        const SuffixMasks m = (it != masks.end()) ? it->second : SuffixMasks{};

        FieldMasks fm;
        fm.suffix = suffix;
        if (any_frlg_define) {
            // Dual layout: bare defines are primary; FRLG defines (or the FRLG-valued table) are the alternate.
            fm.primary = m.bare_define;
            fm.frlg = m.frlg_define.has_value() ? m.frlg_define : m.array_value;
            if (m.frlg_define.has_value() && m.array_value.has_value() &&
                m.frlg_define.value() != m.array_value.value()) {
                result.warnings.push_back(format->format(
                    "field '{}' FRLG mask define disagrees with the mask table; using the define",
                    FormatParam{to_lower(suffix), Style::bold}));
            }
        }
        else {
            // Single layout: a define or the table gives the primary mask; the define wins on conflict.
            if (m.bare_define.has_value()) {
                fm.primary = m.bare_define;
                if (m.array_value.has_value() && m.array_value.value() != m.bare_define.value()) {
                    result.warnings.push_back(format->format(
                        "field '{}' mask define disagrees with the mask table; using the define",
                        FormatParam{to_lower(suffix), Style::bold}));
                }
            }
            else {
                fm.primary = m.array_value;
            }
        }
        field_masks.push_back(fm);
    }

    // --- Phase B and C: name, attach providers, and fill or reject masks ---

    // The stock two-byte layout exception applies when the whole discovered field set is exactly behavior + layer.
    const bool behavior_only_two_byte = scan.detected_attribute_size == 2 && ordered_suffixes.size() == 2 &&
                                        seen_suffixes.contains("BEHAVIOR") && seen_suffixes.contains("LAYER_TYPE");

    for (auto &fm : field_masks) {
        const std::string &suffix = fm.suffix;

        // Phase B: the layer-type field is structural, never emitted as an attribute field. Its mask is still
        // recorded so downstream resolution can honor a base game's custom layer-type position instead of assuming
        // the size-based default.
        if (suffix == "LAYER_TYPE") {
            result.layer_type_mask = fm.primary;
            result.layer_type_frlg_mask = fm.frlg;

            // Same shift-table cross-check applied to real fields: the recorded shift should equal the mask offset.
            if (const auto shift_it = shifts.find(suffix); shift_it != shifts.end()) {
                const auto mask_for_check = any_frlg_define ? fm.frlg : fm.primary;
                if (mask_for_check.has_value()) {
                    const auto expected = static_cast<std::uint32_t>(std::countr_zero(mask_for_check.value()));
                    if (shift_it->second != expected) {
                        result.warnings.push_back(format->format(
                            "field '{}' shift table entry ({}) does not match its mask offset ({}); using the mask",
                            FormatParam{"layer_type", Style::bold},
                            FormatParam{shift_it->second},
                            FormatParam{expected}));
                    }
                }
            }
            continue;
        }

        std::string field_name;
        std::optional<ProviderSpec> provider;

        if (suffix == "BEHAVIOR") {
            field_name = "behavior";
            if (scan.behaviors_header_present) {
                provider = ProviderSpec{behaviors_header, "MB_", {"MB_INVALID"}, HeaderFormat::either};
            }
            else {
                result.warnings.push_back(format->format(
                    "no behavior constants found in {}; leaving the behavior field without a value provider",
                    FormatParam{behaviors_header, Style::bold}));
            }
        }
        else if (is_all_digits(suffix)) {
            field_name = "attribute_" + suffix;
        }
        else {
            field_name = to_lower(suffix);
            std::string probe = "TILE_" + suffix + "_";
            if (!any_enum_member_has_prefix(scan, probe) && suffix.ends_with("_TYPE")) {
                probe = "TILE_" + suffix.substr(0, suffix.size() - std::string_view{"_TYPE"}.size()) + "_";
            }
            if (any_enum_member_has_prefix(scan, probe)) {
                provider = ProviderSpec{fieldmap_header, probe, {}, HeaderFormat::enums_only};
            }
        }

        // Phase C: resolve a field that carries no mask at all.
        if (!fm.primary.has_value() && !fm.frlg.has_value()) {
            if (suffix == "BEHAVIOR" && behavior_only_two_byte) {
                fm.primary = 0x00FFU; // stock two-byte layout: behavior occupies the low byte
            }
            else {
                result.status = AttributeInferenceStatus::invalid;
                result.error_message = format->format(
                    "could not determine a bit mask for metatile attribute field '{}'. The base game declares this "
                    "field but exposes no mask for it. Provide one of: restore the sMetatileAttrMasks[] table under "
                    "its "
                    "exact name in src/fieldmap.c; add a METATILE_ATTR_{}_MASK #define in {}; or set the mask "
                    "explicitly via metatile_attribute_field_overrides (or a full metatile_attribute_fields list) in "
                    "your "
                    "Porytiles config.",
                    FormatParam{field_name, Style::bold},
                    FormatParam{suffix},
                    FormatParam{fieldmap_header, Style::bold});
                return result;
            }
        }

        // Shift table cross-check: the recorded shift should equal the mask's low-bit offset. The
        // sMetatileAttrMasks/sMetatileAttrShifts tables describe the FRLG layout in a dual-layout project, so the
        // shift must be checked against the FRLG mask offset there; a single-layout project uses the primary mask.
        if (const auto shift_it = shifts.find(suffix); shift_it != shifts.end()) {
            const auto mask_for_check = any_frlg_define ? fm.frlg : fm.primary;
            if (mask_for_check.has_value()) {
                const auto expected = static_cast<std::uint32_t>(std::countr_zero(mask_for_check.value()));
                if (shift_it->second != expected) {
                    result.warnings.push_back(format->format(
                        "field '{}' shift table entry ({}) does not match its mask offset ({}); using the mask",
                        FormatParam{field_name, Style::bold},
                        FormatParam{shift_it->second},
                        FormatParam{expected}));
                }
            }
        }

        MetatileAttributeFieldSpec spec;
        spec.name = std::move(field_name);
        spec.mask = fm.primary;
        spec.frlg_mask = fm.frlg;
        spec.provider = std::move(provider);
        result.fields.push_back(std::move(spec));
    }

    if (result.fields.empty()) {
        // Every discovered suffix was structural (e.g. only LAYER_TYPE); nothing usable to provide.
        result.status = AttributeInferenceStatus::not_provided;
        return result;
    }

    result.status = AttributeInferenceStatus::valid;
    return result;
}

} // namespace porytiles
