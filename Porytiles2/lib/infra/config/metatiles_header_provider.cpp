#include "porytiles2/infra/config/metatiles_header_provider.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

#include "porytiles2/infra/config/layer_value.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace {

using namespace porytiles2;

const std::filesystem::path metatiles_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "metatiles.h";

/**
 * @brief Scans a metatiles.h file and detects the attribute byte size from gMetatileAttributes_ declarations.
 *
 * @details
 * Looks for lines containing "gMetatileAttributes_" and checks whether the type is "const u16" or "const u32".
 * Returns a LayerValue with the detected size, or not_provided if the file is missing or has no attribute lines.
 */
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

    bool found_u16 = false;
    bool found_u32 = false;
    std::string line;

    while (std::getline(in, line)) {
        if (line.find("gMetatileAttributes_") == std::string::npos) {
            continue;
        }

        if (line.find("const u16") != std::string::npos) {
            found_u16 = true;
        }
        if (line.find("const u32") != std::string::npos) {
            found_u32 = true;
        }
    }

    if (!found_u16 && !found_u32) {
        return LayerValue<std::size_t>::not_provided();
    }

    const std::string source_info = metatiles_path.string();

    if (found_u16 && found_u32) {
        return LayerValue<std::size_t>::invalid(
            format->format(
                "Mixed u16/u32 attribute declarations found in '{}'.",
                FormatParam{metatiles_path.string(), Style::bold}),
            source_info);
    }

    const std::size_t attr_size = found_u32 ? 4 : 2;
    return LayerValue<std::size_t>::valid(attr_size, "MetatilesHeaderProvider", source_info);
}

} // namespace

namespace porytiles2 {

std::string MetatilesHeaderProvider::name() const
{
    return "MetatilesHeaderProvider";
}

LayerValue<std::size_t>
MetatilesHeaderProvider::metatile_attr_size(ConfigScopeType /*type*/, const std::string & /*scope*/) const
{
    if (!cached_result_.has_value()) {
        cached_result_ = detect_attr_size(project_root_ / metatiles_rel_path, format_);
    }
    return cached_result_.value();
}

} // namespace porytiles2
