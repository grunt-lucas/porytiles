#include "porytiles/infra/services/metatile_attribute_scanner.hpp"

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <utility>

#include "porytiles/utilities/c_parser/c_parser_facade.hpp"

namespace porytiles {

namespace {

const std::filesystem::path fieldmap_header_rel = std::filesystem::path{"include"} / "global.fieldmap.h";
const std::filesystem::path fieldmap_source_rel = std::filesystem::path{"src"} / "fieldmap.c";
const std::filesystem::path behaviors_header_rel =
    std::filesystem::path{"include"} / "constants" / "metatile_behaviors.h";

constexpr const char *masks_array_name = "sMetatileAttrMasks";
constexpr const char *shifts_array_name = "sMetatileAttrShifts";
constexpr const char *tileset_struct_name = "Tileset";
constexpr const char *attributes_member_name = "metatileAttributes";

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

// Extracts the raw pointed-to type of struct Tileset's metatileAttributes member. Every base game declares the
// member as 'const uN *metatileAttributes'; anything else (missing struct, missing member, non-pointer) yields
// nullopt. What the type name means is the domain's call, not the scanner's.
[[nodiscard]] std::optional<std::string> attributes_element_type_from(const std::vector<StructDefinition> &defs)
{
    for (const auto &def : defs) {
        if (def.name != tileset_struct_name) {
            continue;
        }
        for (const auto &member : def.members) {
            if (member.member_name == attributes_member_name && member.pointer_depth == 1) {
                return member.type_name;
            }
        }
    }
    return std::nullopt;
}

} // namespace

MetatileAttributeScanner::MetatileAttributeScanner(
    std::filesystem::path project_root, gsl::not_null<const TextFormatter *> format)
    : project_root_{std::move(project_root)}, format_{format}
{
}

MetatileAttributeScanOutcome MetatileAttributeScanner::scan_project() const
{
    MetatileAttributeScanOutcome outcome;

    const auto fieldmap_header = project_root_ / fieldmap_header_rel;
    outcome.source = fieldmap_header.string();
    if (!std::filesystem::exists(fieldmap_header)) {
        // No fieldmap header: the project states nothing about its attribute layout.
        return outcome;
    }

    CParserFacade header_facade{fieldmap_header, format_};
    auto defines_result = header_facade.parse_defines_tolerant();
    if (!defines_result.has_value()) {
        outcome.warnings.push_back(format_->format(
            "Could not scan '{}' for attribute masks; skipping schema inference.",
            FormatParam{fieldmap_header.string(), Style::bold}));
        return outcome;
    }
    auto enums_result = header_facade.parse_enums_tolerant();
    if (!enums_result.has_value()) {
        outcome.warnings.push_back(format_->format(
            "Could not scan '{}' for the attribute enum; skipping schema inference.",
            FormatParam{fieldmap_header.string(), Style::bold}));
        return outcome;
    }

    const auto &header_defines = defines_result.value().defines;
    const auto &header_enums = enums_result.value().enums;

    outcome.scan.ambiguous_defines = defines_result.value().ambiguous_values;
    std::unordered_map<std::string, std::int64_t> seeds;

    for (const auto &define : header_defines) {
        if (define.has_int_value()) {
            outcome.scan.defines.push_back(
                InferenceDefine{define.name(), static_cast<std::uint32_t>(define.int_value())});
            seeds[define.name()] = define.int_value();
        }
    }
    for (const auto &decl : header_enums) {
        for (const auto &member : decl.members) {
            outcome.scan.enum_members.push_back(InferenceEnumMember{member.name, member.value});
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
                    outcome.scan.masks_array = to_inference_entries(array.entries);
                }
                else if (array.name == shifts_array_name) {
                    outcome.scan.shifts_array = to_inference_entries(array.entries);
                }
            }
        }
        else {
            // The table file is present but could not be lexed or parsed. Unlike a missing table (which states
            // nothing about the layout), a present-but-unparseable one is worth surfacing as data: without it the
            // header defines alone drive inference, and a table-driven project would otherwise be told its masks are
            // missing and advised to restore a table that already exists.
            outcome.warnings.push_back(format_->format(
                "Could not scan '{}' for the metatile attribute mask and shift tables; continuing from the header "
                "defines only.",
                FormatParam{fieldmap_source.string(), Style::bold}));
        }
        // Surface the source file's scan warnings too, matching how the header facade's warnings are gathered below.
        for (const auto &warning : source_facade.scan_warnings()) {
            outcome.warnings.push_back(warning);
        }
    }

    outcome.scan.behaviors_header_present = behaviors_header_has_entries(project_root_ / behaviors_header_rel, format_);

    // Gather recoverable scan warnings (conflicting redefinitions in undecidable regions, etc.).
    for (const auto &warning : header_facade.scan_warnings()) {
        outcome.warnings.push_back(warning);
    }

    // The raw pointed-to type of struct Tileset's metatileAttributes member, from the same header. A failed struct
    // scan is not an error: the type simply stays unset.
    auto struct_defs = header_facade.parse_struct_definitions(std::string{tileset_struct_name});
    if (struct_defs.has_value()) {
        outcome.scan.attributes_element_type = attributes_element_type_from(struct_defs.value());
    }

    outcome.fieldmap_present = true;
    return outcome;
}

} // namespace porytiles
