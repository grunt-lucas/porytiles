#include "porytiles/infra/config/metatile_attribute_config_provider.hpp"

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include "porytiles/infra/config/metatile_attr_inference.hpp"
#include "porytiles/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles/xcut/config/config_scope_type.hpp"

namespace porytiles {

namespace {

const std::filesystem::path fieldmap_header_rel = std::filesystem::path{"include"} / "global.fieldmap.h";
const std::filesystem::path fieldmap_source_rel = std::filesystem::path{"src"} / "fieldmap.c";
const std::filesystem::path behaviors_header_rel =
    std::filesystem::path{"include"} / "constants" / "metatile_behaviors.h";

constexpr const char *masks_array_name = "sMetatileAttrMasks";
constexpr const char *shifts_array_name = "sMetatileAttrShifts";
constexpr const char *diagnostic_tag = "metatile-attr-inference";

[[nodiscard]] std::vector<InferenceArrayEntry> to_inference_entries(const std::vector<IndexedArrayEntry> &entries)
{
    std::vector<InferenceArrayEntry> out;
    out.reserve(entries.size());
    for (const auto &entry : entries) {
        std::optional<std::uint32_t> value;
        if (entry.value.has_value()) {
            value = static_cast<std::uint32_t>(entry.value.value());
        }
        out.push_back(InferenceArrayEntry{entry.index_name, value});
    }
    return out;
}

// The behaviors header may declare its MB_ constants as #defines (pokefirered) or as enum members (pokeemerald). The
// field is considered to have a usable value provider when either form yields at least one MB_ name.
[[nodiscard]] bool behaviors_header_has_entries(const std::filesystem::path &path, const TextFormatter *format)
{
    if (!std::filesystem::exists(path)) {
        return false;
    }
    CParserFacade facade{path, format};

    auto defines = facade.parse_defines_tolerant();
    if (defines.has_value()) {
        for (const auto &define : defines.value().defines) {
            if (define.name().starts_with("MB_")) {
                return true;
            }
        }
    }

    auto enums = facade.parse_enums_tolerant();
    if (enums.has_value()) {
        for (const auto &decl : enums.value().enums) {
            for (const auto &member : decl.members) {
                if (member.name.starts_with("MB_")) {
                    return true;
                }
            }
        }
    }

    return false;
}

} // namespace

MetatileAttributeConfigProvider::MetatileAttributeConfigProvider(
    std::filesystem::path project_root,
    gsl::not_null<const TextFormatter *> format,
    gsl::not_null<const UserDiagnostics *> diagnostics)
    : project_root_{std::move(project_root)}, format_{format}, diagnostics_{diagnostics},
      metatiles_provider_{project_root_, format}
{
}

std::string MetatileAttributeConfigProvider::name() const
{
    return "MetatileAttributeConfigProvider";
}

LayerValue<MetatileAttrFieldSpecs>
MetatileAttributeConfigProvider::compute(ConfigScopeType type, const std::string &scope) const
{
    const auto fieldmap_header = project_root_ / fieldmap_header_rel;
    if (!std::filesystem::exists(fieldmap_header)) {
        // No fieldmap header to infer from; defer to the next provider.
        return LayerValue<MetatileAttrFieldSpecs>::not_provided();
    }

    CParserFacade header_facade{fieldmap_header, format_};
    auto defines_result = header_facade.parse_defines_tolerant();
    if (!defines_result.has_value()) {
        diagnostics_->warning(
            diagnostic_tag,
            format_->format(
                "could not scan {} for attribute masks; skipping schema inference",
                FormatParam{fieldmap_header.string(), Style::bold}));
        return LayerValue<MetatileAttrFieldSpecs>::not_provided();
    }
    auto enums_result = header_facade.parse_enums_tolerant();
    if (!enums_result.has_value()) {
        diagnostics_->warning(
            diagnostic_tag,
            format_->format(
                "could not scan {} for the attribute enum; skipping schema inference",
                FormatParam{fieldmap_header.string(), Style::bold}));
        return LayerValue<MetatileAttrFieldSpecs>::not_provided();
    }

    const auto &header_defines = defines_result.value().defines;
    const auto &header_enums = enums_result.value().enums;

    MetatileAttrScan scan;
    std::unordered_map<std::string, std::int64_t> seeds;

    for (const auto &define : header_defines) {
        if (define.has_int_value()) {
            scan.defines.push_back(InferenceDefine{define.name(), static_cast<std::uint32_t>(define.int_value())});
            seeds[define.name()] = define.int_value();
        }
    }
    for (const auto &decl : header_enums) {
        for (const auto &member : decl.members) {
            scan.enum_members.push_back(InferenceEnumMember{member.name, member.value});
            if (member.value.has_value()) {
                seeds[member.name] = member.value.value();
            }
        }
    }

    // Read the exact-name mask/shift tables from the source, seeded with the header symbols so their FRLG-macro values
    // resolve. A missing file or missing table simply leaves the tables empty.
    const auto fieldmap_source = project_root_ / fieldmap_source_rel;
    if (std::filesystem::exists(fieldmap_source)) {
        CParserFacade source_facade{fieldmap_source, format_, seeds};
        auto arrays = source_facade.parse_indexed_arrays();
        if (arrays.has_value()) {
            for (const auto &array : arrays.value()) {
                if (array.name == masks_array_name) {
                    scan.masks_array = to_inference_entries(array.entries);
                }
                else if (array.name == shifts_array_name) {
                    scan.shifts_array = to_inference_entries(array.entries);
                }
            }
        }
    }

    scan.behaviors_header_present = behaviors_header_has_entries(project_root_ / behaviors_header_rel, format_);

    const auto detected_size = metatiles_provider_.detect();
    if (detected_size.state == ValidationState::valid && detected_size.value.has_value()) {
        scan.detected_attr_size = detected_size.value.value();
    }

    // Surface recoverable scan warnings (conflicting redefinitions in undecidable regions, etc.).
    for (const auto &warning : header_facade.scan_warnings()) {
        diagnostics_->warning(diagnostic_tag, warning);
    }

    const auto inference = infer_metatile_attr_fields(scan, format_);
    for (const auto &warning : inference.warnings) {
        diagnostics_->warning(diagnostic_tag, warning);
    }

    switch (inference.status) {
    case AttrInferenceStatus::valid:
        return LayerValue<MetatileAttrFieldSpecs>::valid(
            inference.fields, "MetatileAttributeConfigProvider", fieldmap_header.string());
    case AttrInferenceStatus::invalid:
        return LayerValue<MetatileAttrFieldSpecs>::invalid(inference.error_message, fieldmap_header.string());
    case AttrInferenceStatus::not_provided:
        return LayerValue<MetatileAttrFieldSpecs>::not_provided();
    }
    return LayerValue<MetatileAttrFieldSpecs>::not_provided();
}

LayerValue<MetatileAttrFieldSpecs>
MetatileAttributeConfigProvider::metatile_attr_fields(ConfigScopeType type, const std::string &scope) const
{
    const auto cache_key = to_string(type) + ":" + scope;
    const auto it = cached_results_.find(cache_key);
    if (it != cached_results_.end()) {
        return it->second;
    }
    return cached_results_.emplace(cache_key, compute(type, scope)).first->second;
}

} // namespace porytiles
