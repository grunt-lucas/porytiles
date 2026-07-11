#include "porytiles/infra/config/metatiles_header_provider.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

#include "porytiles/infra/config/layer_value.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace {

using namespace porytiles;

const std::filesystem::path metatiles_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "metatiles.h";

/// @brief Scans a metatiles.h file and detects the attribute byte size from gMetatileAttributes_ declarations.
///
/// @details
/// Looks for lines containing "gMetatileAttributes_" and checks whether the type is "const u8", "const u16", or
/// "const u32", mapping to 1, 2, or 4 bytes respectively (matching Porymap, which supports all three widths).
/// A mix of more than one type is a hard error. Returns a LayerValue with the detected size, or not_provided if
/// the file is missing or has no attribute lines.
///
/// Note that the `struct Tileset` definition in include/global.fieldmap.h also encodes this width, via the element
/// type of its `metatileAttributes` pointer field (const u16* in emerald-family projects, const u32* in
/// firered-family ones). We infer from metatiles.h instead for two reasons: it is the file directly coupled to the
/// metatile_attributes.bin artifact Porytiles emits (the INCBIN'd array's element type is literally the on-disk
/// entry width), and Porytiles already scans it for INCBIN declarations, so no new parsing machinery is needed.
/// Reading the struct field type would require parsing the fields of a C struct type definition, which the C parser
/// facade does not currently do.
[[nodiscard]] LayerValue<std::size_t>
detect_attr_size(const std::filesystem::path &metatiles_path, const TextFormatter *format)
{
    if (!std::filesystem::exists(metatiles_path)) {
        return LayerValue<std::size_t>::not_provided();
    }

    std::ifstream in{metatiles_path};
    if (!in.is_open()) {
        return LayerValue<std::size_t>::not_provided();
    }

    bool found_u8 = false;
    bool found_u16 = false;
    bool found_u32 = false;
    std::string line;

    while (std::getline(in, line)) {
        if (line.find("gMetatileAttributes_") == std::string::npos) {
            continue;
        }

        // "const u8" is not a substring of "const u16"/"const u32", so these checks stay mutually exclusive.
        if (line.find("const u8") != std::string::npos) {
            found_u8 = true;
        }
        if (line.find("const u16") != std::string::npos) {
            found_u16 = true;
        }
        if (line.find("const u32") != std::string::npos) {
            found_u32 = true;
        }
    }

    if (!found_u8 && !found_u16 && !found_u32) {
        return LayerValue<std::size_t>::not_provided();
    }

    const std::string source_info = metatiles_path.string();

    const int distinct_types = static_cast<int>(found_u8) + static_cast<int>(found_u16) + static_cast<int>(found_u32);
    if (distinct_types > 1) {
        return LayerValue<std::size_t>::invalid(
            format->format(
                "Mixed u8/u16/u32 attribute declarations found in '{}'.",
                FormatParam{metatiles_path.string(), Style::bold}),
            source_info);
    }

    const std::size_t attr_size = found_u32 ? 4 : (found_u16 ? 2 : 1);
    return LayerValue<std::size_t>::valid(attr_size, "MetatilesHeaderProvider", source_info);
}

} // namespace

namespace porytiles {

LayerValue<std::size_t> MetatilesHeaderProvider::detect() const
{
    if (!cached_result_.has_value()) {
        cached_result_ = detect_attr_size(project_root_ / metatiles_rel_path, format_);
    }
    return cached_result_.value();
}

} // namespace porytiles
