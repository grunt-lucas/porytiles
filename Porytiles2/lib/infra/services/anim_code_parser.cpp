#include "porytiles2/infra/services/anim_code_parser.hpp"

#include <optional>
#include <set>

#include "fmt/format.h"

#include "porytiles2/utilities/c_parser/c_parser_driver.hpp"
#include "porytiles2/utilities/c_parser/token.hpp"
#include "porytiles2/utilities/string_utils.hpp"
#include "porytiles2/utilities/text/file_highlight_printer.hpp"

namespace porytiles2 {

namespace {

/**
 * @brief Extracts the PascalCase animation name from an identifier.
 *
 * @details
 * For Porytiles-managed format: "gTilesetAnims_PorytilesManaged_General_Flower" -> "Flower"
 * For vanilla format: "gTilesetAnims_General_Flower" -> "Flower"
 *
 * @param identifier The full identifier name
 * @param tileset_name The tileset name to help locate the animation name portion
 * @return The animation name, or empty string if not found
 */
[[nodiscard]] std::string extract_anim_name(const std::string &identifier, const std::string &tileset_name)
{
    // Try Porytiles-managed format: gTilesetAnims_PorytilesManaged_TilesetName_AnimName
    std::string porytiles_prefix = "gTilesetAnims_PorytilesManaged_" + tileset_name + "_";
    if (identifier.starts_with(porytiles_prefix)) {
        std::string remainder = identifier.substr(porytiles_prefix.size());
        // Remove _Frame suffix if present
        auto frame_pos = remainder.find("_Frame");
        if (frame_pos != std::string::npos) {
            return remainder.substr(0, frame_pos);
        }
        return remainder;
    }

    // Try vanilla format: gTilesetAnims_TilesetName_AnimName
    std::string vanilla_prefix = "gTilesetAnims_" + tileset_name + "_";
    if (identifier.starts_with(vanilla_prefix)) {
        std::string remainder = identifier.substr(vanilla_prefix.size());
        // Remove _Frame suffix if present
        auto frame_pos = remainder.find("_Frame");
        if (frame_pos != std::string::npos) {
            return remainder.substr(0, frame_pos);
        }
        // Remove _VDests suffix if present
        auto vdests_pos = remainder.find("_VDests");
        if (vdests_pos != std::string::npos) {
            return remainder.substr(0, vdests_pos);
        }
        return remainder;
    }

    return {};
}

/**
 * @brief Extracts the animation name from a QueueAnimTiles function name.
 *
 * @details
 * For Porytiles-managed format: "QueueAnimTiles_PorytilesManaged_General_Flower" -> "Flower"
 * For vanilla format: "QueueAnimTiles_General_Flower" -> "Flower"
 *
 * @param func_name The function name
 * @param tileset_name The tileset name to help locate the animation name portion
 * @return The animation name, or empty string if not a matching function
 */
[[nodiscard]] std::string extract_anim_name_from_function(const std::string &func_name, const std::string &tileset_name)
{
    // Try Porytiles-managed format: QueueAnimTiles_PorytilesManaged_TilesetName_AnimName
    std::string porytiles_prefix = "QueueAnimTiles_PorytilesManaged_" + tileset_name + "_";
    if (func_name.starts_with(porytiles_prefix)) {
        return func_name.substr(porytiles_prefix.size());
    }

    // Try vanilla format: QueueAnimTiles_TilesetName_AnimName
    std::string vanilla_prefix = "QueueAnimTiles_" + tileset_name + "_";
    if (func_name.starts_with(vanilla_prefix)) {
        return func_name.substr(vanilla_prefix.size());
    }

    return {};
}

/**
 * @brief Extracts frame indices from array element names.
 *
 * @details
 * Given element names like ["..._Frame0", "..._Frame1", "..._Frame0", "..._Frame2"],
 * returns the frame indices [0, 1, 0, 2].
 *
 * @param elements Vector of element identifier names
 * @return Vector of frame indices in order
 */
[[nodiscard]] std::vector<std::size_t> extract_frame_indices(const std::vector<std::string> &elements)
{
    std::vector<std::size_t> frames;
    frames.reserve(elements.size());

    for (const auto &elem : elements) {
        // Find "_Frame" suffix and extract the digit
        auto frame_pos = elem.find("_Frame");
        if (frame_pos != std::string::npos) {
            std::string frame_str = elem.substr(frame_pos + 6); // Skip "_Frame"
            try {
                frames.push_back(std::stoull(frame_str));
            }
            catch (...) {
                // Skip elements that don't match the pattern
            }
        }
    }

    return frames;
}

/**
 * @brief Finds TILE_OFFSET_4BPP(X) in function body tokens and returns X.
 *
 * @details
 * Searches for the token pattern: identifier("TILE_OFFSET_4BPP") + lparen + integer + rparen
 *
 * @param body_tokens The function body tokens
 * @return The tile offset value, or nullopt if not found
 */
[[nodiscard]] std::optional<std::size_t> extract_tile_offset(const std::vector<Token> &body_tokens)
{
    for (std::size_t i = 0; i + 3 < body_tokens.size(); ++i) {
        if (body_tokens[i].is(TokenType::identifier) && body_tokens[i].text() == "TILE_OFFSET_4BPP" &&
            body_tokens[i + 1].is(TokenType::left_paren) && body_tokens[i + 2].is(TokenType::integer_literal) &&
            body_tokens[i + 3].is(TokenType::right_paren)) {
            return static_cast<std::size_t>(body_tokens[i + 2].int_value());
        }
    }
    return std::nullopt;
}

/**
 * @brief Finds X * TILE_SIZE_4BPP in function body tokens and returns X.
 *
 * @details
 * Searches for the token pattern: integer + star + identifier("TILE_SIZE_4BPP")
 *
 * @param body_tokens The function body tokens
 * @return The tile count value, or nullopt if not found
 */
[[nodiscard]] std::optional<std::size_t> extract_tile_count(const std::vector<Token> &body_tokens)
{
    for (std::size_t i = 0; i + 2 < body_tokens.size(); ++i) {
        if (body_tokens[i].is(TokenType::integer_literal) && body_tokens[i + 1].is(TokenType::star) &&
            body_tokens[i + 2].is(TokenType::identifier) && body_tokens[i + 2].text() == "TILE_SIZE_4BPP") {
            return static_cast<std::size_t>(body_tokens[i].int_value());
        }
    }
    return std::nullopt;
}

/**
 * @brief Extracts timer conditions from driver function body.
 *
 * @details
 * Searches for patterns like: if (timer % X == Y) { ... QueueAnimTiles_..._AnimName
 * Returns a map of animation name -> (frame_factor, frame_offset)
 *
 * @param body_tokens The driver function body tokens
 * @param tileset_name The tileset name for identifying animation calls
 * @return Map of animation names to their (frame_factor, frame_offset) pairs
 */
[[nodiscard]] std::map<std::string, std::pair<std::size_t, std::size_t>>
extract_timer_conditions(const std::vector<Token> &body_tokens, const std::string &tileset_name)
{
    std::map<std::string, std::pair<std::size_t, std::size_t>> result;

    // Look for: timer % X == Y patterns followed by QueueAnimTiles calls
    for (std::size_t i = 0; i + 6 < body_tokens.size(); ++i) {
        // Check for: timer % X == Y
        if (body_tokens[i].is(TokenType::identifier) && body_tokens[i].text() == "timer" &&
            body_tokens[i + 1].is(TokenType::percent) && body_tokens[i + 2].is(TokenType::integer_literal) &&
            body_tokens[i + 3].is(TokenType::equal_equal) && body_tokens[i + 4].is(TokenType::integer_literal)) {

            std::size_t frame_factor = static_cast<std::size_t>(body_tokens[i + 2].int_value());
            std::size_t frame_offset = static_cast<std::size_t>(body_tokens[i + 4].int_value());

            // Search ahead for QueueAnimTiles call
            for (std::size_t j = i + 5; j < body_tokens.size() && j < i + 50; ++j) {
                if (body_tokens[j].is(TokenType::identifier) && body_tokens[j].text().starts_with("QueueAnimTiles_")) {
                    std::string anim_name = extract_anim_name_from_function(body_tokens[j].text(), tileset_name);
                    if (!anim_name.empty()) {
                        result[anim_name] = {frame_factor, frame_offset};
                    }
                    break;
                }

                // Stop if we hit another 'if' - we've gone past the relevant block
                if (body_tokens[j].is(TokenType::kw_if)) {
                    break;
                }
            }
        }
    }

    return result;
}

/**
 * @brief Creates an error with source file context.
 *
 * @param file_lines The source file lines
 * @param position The source position of the error
 * @param format The text formatter
 * @param file_path The file path for the error message
 * @param message The error message
 * @return FormattableError with source context
 */
[[nodiscard]] FormattableError make_highlighted_error(
    const std::vector<std::string> &file_lines,
    const SourcePosition &position,
    const TextFormatter *format,
    const std::string &file_path,
    const std::string &message)
{
    FileHighlightPrinter printer{format};

    // Build error details
    std::vector<std::string> details;
    details.push_back(fmt::format("{}:{}:{}: {}", file_path, position.line, position.column, message));

    // Add source context if position is valid
    if (position.line > 0 && position.line <= file_lines.size()) {
        auto context = printer.print(file_lines, position.line - 1, position.column - 1);
        details.insert(details.end(), context.begin(), context.end());
    }

    return FormattableError{details};
}

} // namespace

AnimCodeParser::AnimCodeParser(gsl::not_null<const TextFormatter *> format) : format_{format} {}

ChainableResult<std::map<std::string, AnimationParams>>
AnimCodeParser::parse_generated_header(const std::filesystem::path &header_path) const
{
    CParserDriver driver{header_path, format_};

    // Parse pointer arrays to get animation frame sequences
    auto arrays_result = driver.parse_pointer_arrays();
    if (!arrays_result.has_value()) {
        return ChainableResult<std::map<std::string, AnimationParams>>{
            FormattableError{format_->format(
                "{}: failed to parse animation frame arrays", FormatParam{header_path.string(), Style::bold})},
            arrays_result};
    }

    // Parse functions to get QueueAnimTiles and driver functions
    auto funcs_result = driver.parse_functions();
    if (!funcs_result.has_value()) {
        return ChainableResult<std::map<std::string, AnimationParams>>{
            FormattableError{format_->format(
                "{}: failed to parse animation functions", FormatParam{header_path.string(), Style::bold})},
            funcs_result};
    }

    const auto &arrays = arrays_result.value();
    const auto &funcs = funcs_result.value();

    std::map<std::string, AnimationParams> result;

    // Extract tileset name from file path or arrays
    std::string tileset_name;
    for (const auto &arr : arrays) {
        if (arr.name().starts_with("gTilesetAnims_PorytilesManaged_")) {
            // Extract tileset name: gTilesetAnims_PorytilesManaged_<TilesetName>_<AnimName>
            std::string remainder = arr.name().substr(31); // Skip "gTilesetAnims_PorytilesManaged_"
            auto underscore_pos = remainder.find('_');
            if (underscore_pos != std::string::npos) {
                tileset_name = remainder.substr(0, underscore_pos);
                break;
            }
        }
    }

    if (tileset_name.empty()) {
        return FormattableError{
            format_->format("{}: could not determine tileset name", FormatParam{header_path.string(), Style::bold})};
    }

    // Find animation names from pointer arrays (those without _Frame suffix are the main arrays)
    std::set<std::string> anim_names;
    for (const auto &arr : arrays) {
        std::string anim_name = extract_anim_name(arr.name(), tileset_name);
        if (!anim_name.empty() && arr.name().find("_Frame") == std::string::npos) {
            anim_names.insert(anim_name);
        }
    }

    // Find driver function to extract timer conditions
    std::map<std::string, std::pair<std::size_t, std::size_t>> timer_conditions;
    for (const auto &func : funcs) {
        if (func.name().starts_with("TilesetAnim_PorytilesManaged_")) {
            timer_conditions = extract_timer_conditions(func.body_tokens(), tileset_name);
            break;
        }
    }

    // Build AnimationParams for each animation
    for (const auto &anim_name : anim_names) {
        AnimationParams params;

        // Convert PascalCase anim name to snake_case for the result key
        std::string result_key;
        for (std::size_t i = 0; i < anim_name.size(); ++i) {
            if (i > 0 && std::isupper(static_cast<unsigned char>(anim_name[i]))) {
                result_key += '_';
            }
            result_key += static_cast<char>(std::tolower(static_cast<unsigned char>(anim_name[i])));
        }

        // Find the frame sequence from the pointer array
        for (const auto &arr : arrays) {
            std::string arr_anim_name = extract_anim_name(arr.name(), tileset_name);
            if (arr_anim_name == anim_name && arr.name().find("_Frame") == std::string::npos) {
                auto frames = extract_frame_indices(arr.elements());
                if (!frames.empty()) {
                    params.frames(std::move(frames));
                }
                break;
            }
        }

        // Find tile_offset and tile_count from the QueueAnimTiles function
        for (const auto &func : funcs) {
            std::string func_anim_name = extract_anim_name_from_function(func.name(), tileset_name);
            if (func_anim_name == anim_name) {
                auto tile_offset = extract_tile_offset(func.body_tokens());
                if (tile_offset.has_value()) {
                    params.tile_offset(tile_offset.value());
                }
                else {
                    return make_highlighted_error(
                        driver.file_lines(),
                        func.position(),
                        format_,
                        header_path.string(),
                        fmt::format("QueueAnimTiles function '{}' missing TILE_OFFSET_4BPP call", func.name()));
                }

                auto tile_count = extract_tile_count(func.body_tokens());
                if (tile_count.has_value()) {
                    params.tile_count(tile_count.value());
                }
                else {
                    return make_highlighted_error(
                        driver.file_lines(),
                        func.position(),
                        format_,
                        header_path.string(),
                        fmt::format("QueueAnimTiles function '{}' missing TILE_SIZE_4BPP expression", func.name()));
                }
                break;
            }
        }

        // Get timer conditions from the driver function
        auto it = timer_conditions.find(anim_name);
        if (it != timer_conditions.end()) {
            params.frame_factor(it->second.first);
            params.frame_offset(it->second.second);
        }
        else {
            // Use defaults
            params.frame_factor(anim::default_frame_factor);
            params.frame_offset(anim::default_frame_offset);
        }

        result[result_key] = std::move(params);
    }

    return result;
}

ChainableResult<std::map<std::string, AnimationParams>>
AnimCodeParser::parse_vanilla_anims(const std::filesystem::path &anims_c_path, const std::string &tileset_name) const
{
    CParserDriver driver{anims_c_path, format_};

    const std::string pascal_case_tileset_name = to_pascal_case(tileset_name);

    // Parse pointer arrays to get animation frame sequences
    auto arrays_result = driver.parse_pointer_arrays();
    if (!arrays_result.has_value()) {
        return ChainableResult<std::map<std::string, AnimationParams>>{
            FormattableError{format_->format(
                "{}: failed to parse animation frame arrays", FormatParam{anims_c_path.string(), Style::bold})},
            arrays_result};
    }

    // Parse functions to get QueueAnimTiles and driver functions
    std::string queue_prefix = "QueueAnimTiles_" + pascal_case_tileset_name + "_";
    auto funcs_result = driver.parse_functions(queue_prefix);
    if (!funcs_result.has_value()) {
        return ChainableResult<std::map<std::string, AnimationParams>>{
            FormattableError{format_->format(
                "{}: failed to parse animation functions", FormatParam{anims_c_path.string(), Style::bold})},
            funcs_result};
    }

    const auto &arrays = arrays_result.value();
    const auto &funcs = funcs_result.value();

    std::map<std::string, AnimationParams> result;

    // Find animation names from QueueAnimTiles functions
    std::set<std::string> anim_names;
    for (const auto &func : funcs) {
        std::string anim_name = extract_anim_name_from_function(func.name(), pascal_case_tileset_name);
        if (!anim_name.empty()) {
            anim_names.insert(anim_name);
        }
    }

    // Parse the driver function to get timer conditions
    // We need to find the TilesetAnim_<TilesetName> or similar driver function
    auto all_funcs_result = driver.parse_functions();
    if (!all_funcs_result.has_value()) {
        return ChainableResult<std::map<std::string, AnimationParams>>{
            FormattableError{format_->format(
                "{}: failed to parse driver functions", FormatParam{anims_c_path.string(), Style::bold})},
            all_funcs_result};
    }

    std::map<std::string, std::pair<std::size_t, std::size_t>> timer_conditions;
    for (const auto &func : all_funcs_result.value()) {
        // Look for driver function: TilesetAnim_<TilesetName> or similar
        if (func.name().starts_with("TilesetAnim_") &&
            func.name().find(pascal_case_tileset_name) != std::string::npos) {
            timer_conditions = extract_timer_conditions(func.body_tokens(), pascal_case_tileset_name);
            break;
        }
    }

    // Build AnimationParams for each animation
    for (const auto &anim_name : anim_names) {
        AnimationParams params;

        // Find the frame sequence from the pointer array
        for (const auto &arr : arrays) {
            std::string arr_anim_name = extract_anim_name(arr.name(), pascal_case_tileset_name);
            if (arr_anim_name == anim_name && arr.name().find("_Frame") == std::string::npos) {
                auto frames = extract_frame_indices(arr.elements());
                if (!frames.empty()) {
                    params.frames(std::move(frames));
                }
                break;
            }
        }

        // Find tile_offset and tile_count from the QueueAnimTiles function
        for (const auto &func : funcs) {
            std::string func_anim_name = extract_anim_name_from_function(func.name(), pascal_case_tileset_name);
            if (func_anim_name == anim_name) {
                auto tile_offset = extract_tile_offset(func.body_tokens());
                if (tile_offset.has_value()) {
                    params.tile_offset(tile_offset.value());
                }

                auto tile_count = extract_tile_count(func.body_tokens());
                if (tile_count.has_value()) {
                    params.tile_count(tile_count.value());
                }
                break;
            }
        }

        // Get timer conditions from the driver function
        auto it = timer_conditions.find(anim_name);
        if (it != timer_conditions.end()) {
            params.frame_factor(it->second.first);
            params.frame_offset(it->second.second);
        }
        else {
            // Use defaults
            params.frame_factor(anim::default_frame_factor);
            params.frame_offset(anim::default_frame_offset);
        }

        result[anim_name] = std::move(params);
    }

    return result;
}

} // namespace porytiles2
