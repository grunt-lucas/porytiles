#include "porytiles2/infra/services/anim_code_parser.hpp"

#include <fstream>
#include <regex>
#include <set>
#include <sstream>

#include "fmt/format.h"

namespace porytiles2 {

/*
 * TODO: all of this should instead utilize our CParserDriver class. We'll need to add some additional use cases to the
 * CParserDriver to support the anim parser's needs.
 */

ChainableResult<std::map<std::string, AnimationParams>>
AnimCodeParser::parse_generated_header(const std::filesystem::path &header_path) const
{
    if (!std::filesystem::exists(header_path)) {
        return FormattableError{
            "{}: generated animation header not found", FormatParam{header_path.string(), Style::bold}};
    }

    std::ifstream file{header_path};
    if (!file) {
        return FormattableError{
            "{}: failed to open generated animation header", FormatParam{header_path.string(), Style::bold}};
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return parse_generated_header_content(buffer.str());
}

ChainableResult<std::map<std::string, AnimationParams>>
AnimCodeParser::parse_generated_header_content(const std::string &content) const
{
    std::map<std::string, AnimationParams> result;

    // Extract animation names from content
    auto anim_names = extract_animation_names(content);

    for (const auto &anim_name : anim_names) {
        AnimationParams params;

        // Parse tile_offset
        auto tile_offset = parse_tile_offset(content, anim_name);
        if (tile_offset.has_value()) {
            params.tile_offset(tile_offset.value());
        }

        // Parse tile_count
        auto tile_count = parse_tile_count(content, anim_name);
        if (tile_count.has_value()) {
            params.tile_count(tile_count.value());
        }

        // Parse frame_factor and frame_offset
        auto [frame_factor, frame_offset] = parse_frame_timing(content, anim_name);
        params.frame_factor(frame_factor);
        params.frame_offset(frame_offset);

        // Parse frames array
        auto frames = parse_frames_array(content, anim_name);
        if (!frames.empty()) {
            params.frames(std::move(frames));
        }

        result[anim_name] = std::move(params);
    }

    return result;
}

ChainableResult<std::map<std::string, AnimationParams>>
AnimCodeParser::parse_vanilla_anims(const std::filesystem::path &anims_c_path, const std::string &tileset_name) const
{
    if (!std::filesystem::exists(anims_c_path)) {
        return FormattableError{"tileset_anims.c not found: {}", FormatParam{anims_c_path.string(), Style::bold}};
    }

    std::ifstream file{anims_c_path};
    if (!file) {
        return FormattableError{"failed to open tileset_anims.c: {}", FormatParam{anims_c_path.string(), Style::bold}};
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();

    std::map<std::string, AnimationParams> result;

    // Look for QueueAnimTiles functions matching this tileset
    // Pattern: QueueAnimTiles_TilesetName_AnimName
    std::regex queue_func_regex(
        fmt::format(R"(static void QueueAnimTiles_{}_([\w]+)\s*\()", tileset_name), std::regex_constants::ECMAScript);

    std::set<std::string> found_anims;
    std::sregex_iterator iter(content.begin(), content.end(), queue_func_regex);
    std::sregex_iterator end;

    while (iter != end) {
        std::string anim_name = (*iter)[1].str();
        found_anims.insert(anim_name);
        ++iter;
    }

    // For each animation found, extract parameters
    for (const auto &anim_name : found_anims) {
        AnimationParams params;

        // Parse tile_offset from TILE_OFFSET_4BPP(X) pattern
        std::regex offset_regex(
            fmt::format(
                R"(QueueAnimTiles_{}_{}\s*\([^)]*\)[^{{]*\{{[^}}]*TILE_OFFSET_4BPP\s*\(\s*(\d+)\s*\))",
                tileset_name,
                anim_name));
        std::smatch offset_match;
        if (std::regex_search(content, offset_match, offset_regex)) {
            params.tile_offset(std::stoull(offset_match[1].str()));
        }

        // Parse tile_count from X * TILE_SIZE_4BPP pattern
        std::regex count_regex(
            fmt::format(
                R"(QueueAnimTiles_{}_{}\s*\([^)]*\)[^{{]*\{{[^}}]*(\d+)\s*\*\s*TILE_SIZE_4BPP)",
                tileset_name,
                anim_name));
        std::smatch count_match;
        if (std::regex_search(content, count_match, count_regex)) {
            params.tile_count(std::stoull(count_match[1].str()));
        }

        // Look for frame array definition
        std::regex array_regex(
            fmt::format(R"(gTilesetAnims_{}_{}(?:_VDests)?\s*\[\s*\]\s*=\s*\{{([^}}]+)\}})", tileset_name, anim_name));
        std::smatch array_match;
        if (std::regex_search(content, array_match, array_regex)) {
            // Count how many Frame entries there are
            std::string array_content = array_match[1].str();
            std::regex frame_ref_regex(R"(_Frame(\d+))");
            std::vector<std::size_t> frames;
            std::sregex_iterator frame_iter(array_content.begin(), array_content.end(), frame_ref_regex);
            while (frame_iter != end) {
                frames.push_back(std::stoull((*frame_iter)[1].str()));
                ++frame_iter;
            }
            if (!frames.empty()) {
                params.frames(std::move(frames));
            }
        }

        // Look for driver function to get frame_factor and frame_offset
        // Pattern: if (timer % X == Y) ... QueueAnimTiles_TilesetName_AnimName
        std::regex driver_regex(
            fmt::format(
                R"(if\s*\(\s*timer\s*%\s*(\d+)\s*==\s*(\d+)\s*\)[^{{]*\{{[^}}]*QueueAnimTiles_{}_{}\s*\()",
                tileset_name,
                anim_name));
        std::smatch driver_match;
        if (std::regex_search(content, driver_match, driver_regex)) {
            params.frame_factor(std::stoull(driver_match[1].str()));
            params.frame_offset(std::stoull(driver_match[2].str()));
        }

        result[anim_name] = std::move(params);
    }

    return result;
}

std::vector<std::string> AnimCodeParser::extract_animation_names(const std::string &content) const
{
    std::set<std::string> names;

    // Look for PorytilesManaged animation names in INCBIN statements
    // Pattern: gTilesetAnims_PorytilesManaged_TilesetName_AnimName_anim_name_Frame0
    std::regex incbin_regex(R"(gTilesetAnims_PorytilesManaged_\w+_(\w+)_(\w+)_Frame\d+)");

    std::sregex_iterator iter(content.begin(), content.end(), incbin_regex);
    std::sregex_iterator end;

    while (iter != end) {
        // The second capture group is the actual animation name (snake_case)
        std::string anim_name = (*iter)[2].str();
        names.insert(anim_name);
        ++iter;
    }

    return {names.begin(), names.end()};
}

std::optional<std::size_t>
AnimCodeParser::parse_tile_offset(const std::string &content, const std::string &anim_name) const
{
    // Look for TILE_OFFSET_4BPP(X) in the QueueAnimTiles function for this animation
    // The animation name appears as PascalCase in the function name
    std::regex offset_regex(
        fmt::format(
            R"(QueueAnimTiles_PorytilesManaged_\w+_\w+\s*\([^)]*\)[^{{]*\{{[^}}]*TILE_OFFSET_4BPP\s*\(\s*(\d+)\s*\))"));

    // We need to find the function that corresponds to this specific animation
    // Look for the pattern with the animation name
    std::regex specific_regex(
        fmt::format(
            R"(QueueAnimTiles_PorytilesManaged_\w+_(\w+)\s*\([^)]*\)[^{{]*\{{[^}}]*TILE_OFFSET_4BPP\s*\(\s*(\d+)\s*\))"));

    std::sregex_iterator iter(content.begin(), content.end(), specific_regex);
    std::sregex_iterator end;

    while (iter != end) {
        // Check if the PascalCase name matches our animation
        std::string found_name = (*iter)[1].str();
        // Convert anim_name to PascalCase for comparison
        std::string pascal_anim;
        bool capitalize = true;
        for (char c : anim_name) {
            if (c == '_') {
                capitalize = true;
            }
            else {
                pascal_anim += capitalize ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c;
                capitalize = false;
            }
        }
        if (found_name == pascal_anim) {
            return std::stoull((*iter)[2].str());
        }
        ++iter;
    }

    return std::nullopt;
}

std::optional<std::size_t>
AnimCodeParser::parse_tile_count(const std::string &content, const std::string &anim_name) const
{
    // Look for X * TILE_SIZE_4BPP in the QueueAnimTiles function
    std::regex specific_regex(
        fmt::format(
            R"(QueueAnimTiles_PorytilesManaged_\w+_(\w+)\s*\([^)]*\)[^{{]*\{{[^}}]*(\d+)\s*\*\s*TILE_SIZE_4BPP)"));

    std::sregex_iterator iter(content.begin(), content.end(), specific_regex);
    std::sregex_iterator end;

    while (iter != end) {
        std::string found_name = (*iter)[1].str();
        std::string pascal_anim;
        bool capitalize = true;
        for (char c : anim_name) {
            if (c == '_') {
                capitalize = true;
            }
            else {
                pascal_anim += capitalize ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c;
                capitalize = false;
            }
        }
        if (found_name == pascal_anim) {
            return std::stoull((*iter)[2].str());
        }
        ++iter;
    }

    return std::nullopt;
}

std::pair<std::size_t, std::size_t>
AnimCodeParser::parse_frame_timing(const std::string &content, const std::string &anim_name) const
{
    // Default values
    std::size_t frame_factor = anim::default_frame_factor;
    std::size_t frame_offset = anim::default_frame_offset;

    // Convert anim_name to PascalCase
    std::string pascal_anim;
    bool capitalize = true;
    for (char c : anim_name) {
        if (c == '_') {
            capitalize = true;
        }
        else {
            pascal_anim += capitalize ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c;
            capitalize = false;
        }
    }

    // Look for: if (timer % X == Y) { ... QueueAnimTiles_PorytilesManaged_..._AnimName
    std::regex timing_regex(
        fmt::format(
            R"(if\s*\(\s*timer\s*%\s*(\d+)\s*==\s*(\d+)\s*\)[^{{]*\{{[^}}]*QueueAnimTiles_PorytilesManaged_\w+_{})",
            pascal_anim));

    std::smatch match;
    if (std::regex_search(content, match, timing_regex)) {
        frame_factor = std::stoull(match[1].str());
        frame_offset = std::stoull(match[2].str());
    }

    return {frame_factor, frame_offset};
}

std::vector<std::size_t>
AnimCodeParser::parse_frames_array(const std::string &content, const std::string &anim_name) const
{
    std::vector<std::size_t> frames;

    // Convert anim_name to PascalCase
    std::string pascal_anim;
    bool capitalize = true;
    for (char c : anim_name) {
        if (c == '_') {
            capitalize = true;
        }
        else {
            pascal_anim += capitalize ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c;
            capitalize = false;
        }
    }

    // Look for the frame pointer array: gTilesetAnims_PorytilesManaged_TilesetName_AnimName[] = { ... }
    std::regex array_regex(
        fmt::format(R"(gTilesetAnims_PorytilesManaged_\w+_{}\s*\[\s*\]\s*=\s*\{{([^}}]+)\}})", pascal_anim));

    std::smatch array_match;
    if (std::regex_search(content, array_match, array_regex)) {
        std::string array_content = array_match[1].str();

        // Extract frame indices from entries like: ..._Frame0, ..._Frame1, etc.
        std::regex frame_ref_regex(R"(_Frame(\d+))");
        std::sregex_iterator iter(array_content.begin(), array_content.end(), frame_ref_regex);
        std::sregex_iterator end;

        while (iter != end) {
            frames.push_back(std::stoull((*iter)[1].str()));
            ++iter;
        }
    }

    return frames;
}

} // namespace porytiles2