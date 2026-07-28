#include "porytiles/infra/services/metatile_attribute_scanner.hpp"

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <utility>

#include "porytiles/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles/utilities/panic/panic.hpp"

namespace porytiles {

namespace {

constexpr auto masks_array_name = "sMetatileAttrMasks";
constexpr auto shifts_array_name = "sMetatileAttrShifts";
constexpr auto tileset_struct_name = "Tileset";
constexpr auto attributes_member_name = "metatileAttributes";

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

// The behaviors header may declare its MB_ constants as #defines (pokefirered) or as enum members (pokeemerald), so
// both forms are scanned. The scan reports what it found, never what it means: an absent header, one that failed to
// scan, one with no MB_ name, and one declaring at least one are four distinct facts, and the domain decides what
// each of them implies for the behavior field's value provider.
[[nodiscard]] BehaviorsHeaderScan scan_behaviors_header(const std::filesystem::path &path, const TextFormatter *format)
{
    BehaviorsHeaderScan scan;
    scan.path = path.string();
    if (!std::filesystem::exists(path)) {
        scan.source = BehaviorsHeaderSource::absent;
        return scan;
    }
    CParserFacade facade{path, format};

    auto defines = facade.parse_defines_tolerant();
    if (defines.has_value()) {
        for (const auto &define : defines.value().defines) {
            if (define.name().starts_with("MB_")) {
                scan.source = BehaviorsHeaderSource::declared;
                return scan;
            }
        }
    }

    auto enums = facade.parse_enums_tolerant();
    if (enums.has_value()) {
        for (const auto &decl : enums.value().enums) {
            for (const auto &member : decl.members) {
                if (member.name.starts_with("MB_")) {
                    scan.source = BehaviorsHeaderSource::declared;
                    return scan;
                }
            }
        }
    }

    // Both tolerant scans fail only on a load or lex failure. A header that could not be scanned at all may declare
    // any number of MB_ names, so it must not be flattened into "declares none".
    scan.source = (!defines.has_value() && !enums.has_value()) ? BehaviorsHeaderSource::unreadable
                                                               : BehaviorsHeaderSource::no_constants;
    return scan;
}

// Locates struct Tileset's metatileAttributes member and records the declarator it was written with. Every base game
// declares the member as 'const uN *metatileAttributes', but this reports whatever is actually there, including a type
// nobody recognizes: what the declarator means is the domain's call, not the scanner's. A member whose declarator the
// parser cannot pattern-match at all never reaches here, so it reads as no member.
[[nodiscard]] AttributeDeclarationScan declaration_from(const std::vector<StructDefinition> &defs)
{
    bool saw_tileset_struct = false;
    for (const auto &def : defs) {
        if (def.name != tileset_struct_name) {
            continue;
        }
        saw_tileset_struct = true;
        for (const auto &member : def.members) {
            if (member.member_name == attributes_member_name) {
                return AttributeDeclarationScan{
                    AttributeDeclarationSource::declared, member.type_name, member.pointer_depth, member.is_const};
            }
        }
    }
    return AttributeDeclarationScan{
        saw_tileset_struct ? AttributeDeclarationSource::no_attributes_member
                           : AttributeDeclarationSource::no_tileset_struct};
}

} // namespace

MetatileAttributeScanner::MetatileAttributeScanner(
    std::filesystem::path project_root,
    gsl::not_null<const TextFormatter *> format,
    gsl::not_null<const UserDiagnostics *> diag)
    : project_root_{std::move(project_root)}, format_{format}, diag_{diag}
{
}

MetatileAttributeScan MetatileAttributeScanner::scan_project() const
{
    const std::filesystem::path fieldmap_header_rel = std::filesystem::path{"include"} / "global.fieldmap.h";
    const std::filesystem::path fieldmap_source_rel = std::filesystem::path{"src"} / "fieldmap.c";
    const std::filesystem::path behaviors_header_rel =
        std::filesystem::path{"include"} / "constants" / "metatile_behaviors.h";

    MetatileAttributeScan scan;

    const auto fieldmap_header = project_root_ / fieldmap_header_rel;
    scan.header_source = fieldmap_header.string();
    if (!std::filesystem::exists(fieldmap_header)) {
        // No fieldmap header: the project states nothing about its attribute layout.
        scan.declaration.source = AttributeDeclarationSource::no_fieldmap_header;
        return scan;
    }

    CParserFacade header_facade{fieldmap_header, format_};
    auto defines_result = header_facade.parse_defines_tolerant();
    if (!defines_result.has_value()) {
        diag_->warning(
            metatile_attr_inference_tag,
            "Could not scan '{}' for attribute masks; skipping schema inference.",
            FormatParam{fieldmap_header.string(), Style::bold});
        scan.unreadable_sources.push_back(fieldmap_header.string());
        scan.declaration.source = AttributeDeclarationSource::header_unreadable;
        return scan;
    }
    // Both tolerant scans fail only on a load or lex failure, and both run over the same cached content, so the enum
    // scan cannot fail once the define scan has succeeded on this facade. Asserted rather than handled: a graceful
    // branch here would be untestable, and if the facade ever grows a failure mode the two scans do not share, a
    // silent "the project declares no attribute enum" is the worst way to find out.
    auto enums_result = header_facade.parse_enums_tolerant();
    assert_or_panic(
        enums_result.has_value(), "enum scan failed on a fieldmap header whose define scan already succeeded");

    const auto &header_defines = defines_result.value().defines;
    const auto &header_enums = enums_result.value().enums;

    scan.ambiguous_defines = defines_result.value().ambiguous_values;
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
        if (auto arrays = source_facade.parse_indexed_arrays(); arrays.has_value()) {
            for (const auto &array : arrays.value()) {
                if (array.name == masks_array_name) {
                    scan.masks_array = to_inference_entries(array.entries);
                    // Recorded only when the table is actually found, so nothing downstream can point a user at a
                    // file that contributed no masks.
                    scan.masks_table_source = fieldmap_source.string();
                }
                else if (array.name == shifts_array_name) {
                    scan.shifts_array = to_inference_entries(array.entries);
                }
            }
        }
        else {
            // The table file is present but could not be lexed or parsed. Unlike a missing table (which states
            // nothing about the layout), a present-but-unparseable one is worth surfacing: without it the header
            // defines alone drive inference, and a table-driven project would otherwise be told its masks are
            // missing and advised to restore a table that already exists.
            diag_->warning(
                metatile_attr_inference_tag,
                "Could not scan '{}' for the metatile attribute mask and shift tables; continuing from the header "
                "defines only.",
                FormatParam{fieldmap_source.string(), Style::bold});
            scan.unreadable_sources.push_back(fieldmap_source.string());
        }
        // Surface the source file's scan warnings too, matching how the header facade's warnings are emitted below.
        for (const auto &warning : source_facade.scan_warnings()) {
            diag_->warning(metatile_attr_inference_tag, warning);
        }
    }

    scan.behaviors_header = scan_behaviors_header(project_root_ / behaviors_header_rel, format_);
    if (scan.behaviors_header.source == BehaviorsHeaderSource::unreadable) {
        // Matches the fieldmap source handling above: a file that exists but could not be scanned is warned about and
        // listed as unreadable, so the missing facts are never mistaken for facts the project failed to state.
        diag_->warning(
            metatile_attr_inference_tag,
            "Could not scan '{}' for behavior constants.",
            FormatParam{scan.behaviors_header.path, Style::bold});
        scan.unreadable_sources.push_back(scan.behaviors_header.path);
    }

    // Surface the recoverable scan warnings (conflicting redefinitions in undecidable regions, etc.).
    for (const auto &warning : header_facade.scan_warnings()) {
        diag_->warning(metatile_attr_inference_tag, warning);
    }

    // struct Tileset's metatileAttributes declaration, from the same header. The struct scan fails only on a load or
    // lex failure, and both already succeeded on this facade's cached content, so it cannot fail here. Asserted rather
    // than handled for the same reason as the enum scan above, and with more at stake: the declaration width has no
    // second source, so quietly recording "this project declares no struct Tileset" would send the user hunting
    // through a header that declares one perfectly well.
    auto struct_defs = header_facade.parse_struct_definitions(std::string{tileset_struct_name});
    assert_or_panic(
        struct_defs.has_value(), "struct scan failed on a fieldmap header whose define scan already succeeded");
    scan.declaration = declaration_from(struct_defs.value());

    return scan;
}

} // namespace porytiles
