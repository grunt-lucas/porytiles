#include "porytiles2/infra/services/anim_code_parser.hpp"

#include <format>
#include <map>
#include <optional>
#include <set>
#include <string>

#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/domain/models/animation_params.hpp"
#include "porytiles2/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles2/utilities/c_parser/function_call_info.hpp"
#include "porytiles2/utilities/c_parser/token.hpp"
#include "porytiles2/utilities/string_utils.hpp"
#include "porytiles2/utilities/text/file_highlight_printer.hpp"

namespace {

using namespace porytiles2;

constexpr auto anim_parsing_error = "animation-parsing-error";

/**
 * @brief Extracts the PascalCase animation name from a gTilesetAnims array reference.
 *
 * @details
 * Parses identifiers like "gTilesetAnims_General_Flower" or "gTilesetAnims_PorytilesManaged_General_Flower"
 * to extract the animation name portion ("Flower").
 *
 * @param identifier The full identifier name (e.g., from AppendTilesetAnimToBuffer first arg)
 * @param tileset_shorthand The tileset name (e.g., "General")
 * @param porytiles_managed Whether this uses the PorytilesManaged_ prefix
 * @return The animation name in PascalCase, or empty string if not found
 */
[[nodiscard]] std::string extract_anim_name_from_array_ref(
    const std::string &identifier, const std::string &tileset_shorthand, bool porytiles_managed)
{
    // Build expected prefix: gTilesetAnims_[PorytilesManaged_]TilesetName_
    std::string prefix = anim::g_tileset_anims_prefix;
    if (porytiles_managed) {
        prefix += anim::porytiles_managed_prefix;
    }
    prefix += tileset_shorthand + "_";

    if (!identifier.starts_with(prefix)) {
        return {};
    }

    std::string remainder = identifier.substr(prefix.size());

    // Remove _Frame suffix if present (for individual frame arrays)
    if (auto frame_pos = remainder.find("_Frame"); frame_pos != std::string::npos) {
        return remainder.substr(0, frame_pos);
    }

    // Remove _VDests suffix if present
    if (auto vdests_pos = remainder.find("_VDests"); vdests_pos != std::string::npos) {
        return remainder.substr(0, vdests_pos);
    }

    return remainder;
}

/**
 * @brief Extracts frame names from array element names.
 *
 * @details
 * Given element names like ["..._Frame0", "..._Frame1", "..._Frame0", "..._Frame2"],
 * returns the frame names as strings ["0", "1", "0", "2"].
 *
 * @param elements Vector of element identifier names
 * @return Vector of frame names in order (as strings)
 */
[[nodiscard]] std::vector<std::string> extract_frame_names(const std::vector<std::string> &elements)
{
    std::vector<std::string> frames;
    frames.reserve(elements.size());

    for (const auto &elem : elements) {
        // Find "_Frame" suffix and extract the frame name
        auto frame_pos = elem.find("_Frame");
        if (frame_pos != std::string::npos) {
            std::string frame_str = elem.substr(frame_pos + 6); // Skip "_Frame"
            // Validate it's a valid frame name (should be numeric for now)
            try {
                std::stoull(frame_str); // Validate it's a number
                frames.push_back(frame_str);
            }
            catch (...) {
                // Skip elements that don't match the pattern
            }
        }
    }

    return frames;
}

/**
 * @brief Finds TILE_OFFSET_4BPP(X) in tokens and returns X.
 *
 * @details
 * Searches for the token pattern: identifier("TILE_OFFSET_4BPP") + lparen + integer + rparen
 *
 * @param tokens The token sequence to search
 * @return The tile offset value, or nullopt if not found
 */
[[nodiscard]] std::optional<std::size_t> extract_tile_offset(const std::vector<Token> &tokens)
{
    for (std::size_t i = 0; i + 3 < tokens.size(); ++i) {
        if (tokens[i].is(TokenType::identifier) && tokens[i].text() == "TILE_OFFSET_4BPP" &&
            tokens[i + 1].is(TokenType::left_paren) && tokens[i + 2].is(TokenType::integer_literal) &&
            tokens[i + 3].is(TokenType::right_paren)) {
            return tokens[i + 2].int_value();
        }
    }
    return std::nullopt;
}

/**
 * @brief Finds X * TILE_SIZE_4BPP in tokens and returns X.
 *
 * @details
 * Searches for the token pattern: integer + star + identifier("TILE_SIZE_4BPP")
 *
 * @param tokens The token sequence to search
 * @return The tile count value, or nullopt if not found
 */
[[nodiscard]] std::optional<std::size_t> extract_tile_count(const std::vector<Token> &tokens)
{
    for (std::size_t i = 0; i + 2 < tokens.size(); ++i) {
        if (tokens[i].is(TokenType::integer_literal) && tokens[i + 1].is(TokenType::star) &&
            tokens[i + 2].is(TokenType::identifier) && tokens[i + 2].text() == "TILE_SIZE_4BPP") {
            return tokens[i].int_value();
        }
    }
    return std::nullopt;
}

/**
 * @brief Finds the driver function name from an Init callback function body.
 *
 * @details
 * Searches for assignment pattern: sPrimaryTilesetAnimCallback = <driver_func>
 * or: sSecondaryTilesetAnimCallback = <driver_func>
 *
 * @param body_tokens The callback function body tokens
 * @return The driver function name, or empty string if not found
 */
[[nodiscard]] std::string find_driver_function_from_callback(const std::vector<Token> &body_tokens)
{
    for (std::size_t i = 0; i + 2 < body_tokens.size(); ++i) {
        if (body_tokens[i].is(TokenType::identifier) &&
            (body_tokens[i].text() == "sPrimaryTilesetAnimCallback" ||
             body_tokens[i].text() == "sSecondaryTilesetAnimCallback") &&
            body_tokens[i + 1].is(TokenType::equal)) {
            // Find the identifier after the equals sign
            for (std::size_t j = i + 2; j < body_tokens.size(); ++j) {
                if (body_tokens[j].is(TokenType::identifier)) {
                    return body_tokens[j].text();
                }
                // Stop if we hit a semicolon
                if (body_tokens[j].is(TokenType::semicolon)) {
                    break;
                }
            }
        }
    }
    return {};
}

/**
 * @brief Represents a discovered timer condition and its associated function call.
 */
struct TimerCondition {
    std::size_t frame_factor; // The X in timer % X
    std::size_t frame_offset; // The Y in timer % X == Y
    std::string called_func;  // The function called inside the condition block
};

/**
 * @brief Extracts timer conditions and associated function calls from a driver function.
 *
 * @details
 * Searches for patterns like: if (timer % X == Y) { ... func_call(...) ... }
 * Returns a vector of TimerCondition structs.
 *
 * @param body_tokens The driver function body tokens
 * @return Vector of timer conditions with their associated function calls
 */
[[nodiscard]] std::vector<TimerCondition> extract_timer_conditions(const std::vector<Token> &body_tokens)
{
    std::vector<TimerCondition> result;

    // Look for: timer % X == Y patterns followed by function calls
    for (std::size_t i = 0; i + 6 < body_tokens.size(); ++i) {
        // Check for: timer % X == Y
        if (body_tokens[i].is(TokenType::identifier) && body_tokens[i].text() == "timer" &&
            body_tokens[i + 1].is(TokenType::percent) && body_tokens[i + 2].is(TokenType::integer_literal) &&
            body_tokens[i + 3].is(TokenType::equal_equal) && body_tokens[i + 4].is(TokenType::integer_literal)) {

            std::size_t frame_factor = body_tokens[i + 2].int_value();
            std::size_t frame_offset = body_tokens[i + 4].int_value();

            // Search ahead for function call (identifier followed by lparen)
            for (std::size_t j = i + 5; j < body_tokens.size() && j < i + 50; ++j) {
                if (body_tokens[j].is(TokenType::identifier) && j + 1 < body_tokens.size() &&
                    body_tokens[j + 1].is(TokenType::left_paren)) {
                    // Found a function call
                    result.push_back({frame_factor, frame_offset, body_tokens[j].text()});
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
 * @brief Represents animation data discovered from AppendTilesetAnimToBuffer calls.
 */
struct DiscoveredAnimData {
    std::string anim_name_pascal; // PascalCase animation name
    std::size_t tile_offset{};
    std::size_t tile_count{};
    std::size_t frame_factor{};
    std::size_t frame_offset{};
};

/**
 * @brief Extracts animation name from the first argument of AppendTilesetAnimToBuffer.
 *
 * @details
 * The first argument is typically something like: gTilesetAnims_General_Flower[i]
 * We need to extract the identifier before the array subscript.
 *
 * @param arg_tokens The tokens of the first argument
 * @return The identifier name, or empty string if not found
 */
[[nodiscard]] std::string extract_array_name_from_first_arg(const std::vector<Token> &arg_tokens)
{
    // Look for an identifier followed by left_bracket
    for (std::size_t i = 0; i + 1 < arg_tokens.size(); ++i) {
        if (arg_tokens[i].is(TokenType::identifier) && arg_tokens[i + 1].is(TokenType::left_bracket)) {
            return arg_tokens[i].text();
        }
    }

    // If no bracket found, just return the first identifier
    for (const auto &tok : arg_tokens) {
        if (tok.is(TokenType::identifier)) {
            return tok.text();
        }
    }

    return {};
}

[[nodiscard]] std::vector<std::string> make_highlighted_details(
    const SourcePosition &position,
    const TextFormatter &format,
    const std::filesystem::path &file_path,
    const std::string &message)
{
    const FileHighlightPrinter printer{&format};

    // Build error details
    std::vector<std::string> details;
    details.push_back(std::format("{}:{}:{}: {}", file_path.string(), position.line, position.column, message));
    details.emplace_back();

    // Add source context if position is valid
    if (position.line > 0) {
        auto context = printer.print(file_path, position.line - 1, position.column - 1);
        details.insert(details.end(), context.begin(), context.end());
    }

    return details;
}

} // namespace

namespace porytiles2 {

ChainableResult<std::map<std::string, AnimationParams>> AnimCodeParser::parse_from_callback(
    const std::filesystem::path &c_file_path,
    const std::string &callback_func_name,
    const std::string &pascal_case_tileset,
    bool porytiles_managed) const
{
    std::map<std::string, AnimationParams> result;
    CParserFacade c_parser{c_file_path, format_};

    // TODO: do we need this here?
    if (to_pascal_case(pascal_case_tileset) != pascal_case_tileset) {
        panic("param pascal_case_tileset = '" + pascal_case_tileset + "', must be pascal case");
    }

    // Step 1: Parse the callback function to find the driver function
    auto callback_funcs_result = c_parser.parse_functions(callback_func_name);
    if (!callback_funcs_result.has_value()) {
        return ChainableResult<std::map<std::string, AnimationParams>>{
            FormattableError{format_->format(
                "{}: failed to parse callback function", FormatParam{c_file_path.string(), Style::bold})},
            callback_funcs_result};
    }

    const auto &callback_funcs = callback_funcs_result.value();
    if (callback_funcs.empty()) {
        // No callback function found - this tileset may not have animations
        return std::map<std::string, AnimationParams>{};
    }

    if (callback_funcs.size() > 1) {
        diag_->warning(
            anim_parsing_error,
            format_->format(
                "found multiple callback functions matching '{}', using first",
                FormatParam{callback_func_name, Style::bold}));
    }

    const auto &callback_func = callback_funcs.front();
    std::string driver_func_name = find_driver_function_from_callback(callback_func.body_tokens());

    if (driver_func_name.empty()) {
        diag_->warning(
            anim_parsing_error,
            format_->format(
                "could not find driver function assignment in '{}'", FormatParam{callback_func_name, Style::bold}));
        return std::map<std::string, AnimationParams>{};
    }

    // Step 2: Parse the driver function to find timer conditions and queue function calls
    auto driver_funcs_result = c_parser.parse_functions(driver_func_name);
    if (!driver_funcs_result.has_value()) {
        return ChainableResult<std::map<std::string, AnimationParams>>{
            FormattableError{format_->format(
                "{}: failed to parse driver function '{}'",
                FormatParam{c_file_path.string(), Style::bold},
                FormatParam{driver_func_name, Style::bold})},
            driver_funcs_result};
    }

    const auto &driver_funcs = driver_funcs_result.value();
    if (driver_funcs.empty()) {
        diag_->warning(
            anim_parsing_error,
            format_->format("driver function '{}' not found in file", FormatParam{driver_func_name, Style::bold}));
        return std::map<std::string, AnimationParams>{};
    }

    const auto &driver_func = driver_funcs.front();
    std::vector<TimerCondition> timer_conditions = extract_timer_conditions(driver_func.body_tokens());

    if (timer_conditions.empty()) {
        diag_->warning(
            anim_parsing_error,
            format_->format(
                "no timer conditions found in driver function '{}'", FormatParam{driver_func_name, Style::bold}));
        return std::map<std::string, AnimationParams>{};
    }

    // Step 3: Parse all functions in the file to find the queue functions
    auto all_funcs_result = c_parser.parse_functions();
    if (!all_funcs_result.has_value()) {
        return ChainableResult<std::map<std::string, AnimationParams>>{
            FormattableError{
                format_->format("{}: failed to parse functions", FormatParam{c_file_path.string(), Style::bold})},
            all_funcs_result};
    }

    // Build a map of function name -> body tokens for quick lookup
    std::map<std::string, const FunctionDefinition *> func_map;
    for (const auto &func : all_funcs_result.value()) {
        func_map[func.name()] = &func;
    }

    // Step 4: For each timer condition, find the queue function and extract animation data
    std::map<std::string, DiscoveredAnimData> discovered_anims;

    for (const auto &condition : timer_conditions) {
        auto it = func_map.find(condition.called_func);
        if (it == func_map.end()) {
            diag_->warning(
                anim_parsing_error,
                format_->format(
                    "queue function '{}' not found in file", FormatParam{condition.called_func, Style::bold}));
            continue;
        }

        const FunctionDefinition *queue_func = it->second;

        // Find AppendTilesetAnimToBuffer calls in the queue function
        auto append_calls = find_function_calls(queue_func->body_tokens(), "AppendTilesetAnimToBuffer");

        if (append_calls.empty()) {
            diag_->warning(
                anim_parsing_error,
                format_->format(
                    "no AppendTilesetAnimToBuffer calls in queue function '{}'",
                    FormatParam{condition.called_func, Style::bold}));
            continue;
        }

        // Process the first AppendTilesetAnimToBuffer call
        // Note: For VDests patterns, there may be multiple calls - we defer full handling per design decision
        const auto &call = append_calls.front();

        if (call.argument_count() < 3) {
            diag_->warning(
                anim_parsing_error,
                format_->format(
                    "AppendTilesetAnimToBuffer call in '{}' has fewer than 3 arguments",
                    FormatParam{condition.called_func, Style::bold}));
            continue;
        }

        // Extract animation name from first argument (e.g., gTilesetAnims_General_Flower[i])
        std::string array_name = extract_array_name_from_first_arg(call.argument_at(0));
        std::string anim_name_pascal =
            extract_anim_name_from_array_ref(array_name, pascal_case_tileset, porytiles_managed);

        if (anim_name_pascal.empty()) {
            diag_->warning(
                anim_parsing_error,
                format_->format("could not extract animation name from '{}'", FormatParam{array_name, Style::bold}));
            continue;
        }

        // Extract tile_offset from second argument
        auto tile_offset = extract_tile_offset(call.argument_at(1));
        if (!tile_offset.has_value()) {
            diag_->warning(
                anim_parsing_error,
                format_->format(
                    "could not extract TILE_OFFSET_4BPP from AppendTilesetAnimToBuffer call for '{}'",
                    FormatParam{anim_name_pascal, Style::bold}));
            continue;
        }

        // Extract tile_count from third argument
        auto tile_count = extract_tile_count(call.argument_at(2));
        if (!tile_count.has_value()) {
            diag_->warning(
                anim_parsing_error,
                format_->format(
                    "could not extract TILE_SIZE_4BPP from AppendTilesetAnimToBuffer call for '{}'",
                    FormatParam{anim_name_pascal, Style::bold}));
            continue;
        }

        // Log warning for multiple AppendTilesetAnimToBuffer calls (VDests pattern)
        if (append_calls.size() > 1) {
            diag_->warning(
                "vdests-pattern-detected",
                format_->format(
                    "queue function '{}' has multiple AppendTilesetAnimToBuffer calls (VDests pattern); "
                    "using first call only",
                    FormatParam{condition.called_func, Style::bold}));
        }

        // Store discovered animation data
        discovered_anims[anim_name_pascal] = {
            anim_name_pascal, tile_offset.value(), tile_count.value(), condition.frame_factor, condition.frame_offset};
    }

    // Step 5: Parse frame pointer arrays to get frame sequences
    const auto frame_array_prefix = anim::g_tileset_anims_prefix +
                                    (porytiles_managed ? anim::porytiles_managed_prefix : std::string{}) +
                                    pascal_case_tileset;

    auto anim_frame_arrays_result = c_parser.parse_pointer_arrays(frame_array_prefix);
    if (!anim_frame_arrays_result.has_value()) {
        return ChainableResult<std::map<std::string, AnimationParams>>{
            FormattableError{format_->format(
                "{}: failed to parse animation frame arrays", FormatParam{c_file_path.string(), Style::bold})},
            anim_frame_arrays_result};
    }

    // Build AnimationParams for each discovered animation
    for (const auto &[pascal_name, anim_data] : discovered_anims) {
        AnimationParams params;
        params.tile_offset(anim_data.tile_offset);
        params.tile_count(anim_data.tile_count);
        params.frame_factor(anim_data.frame_factor);
        params.frame_offset(anim_data.frame_offset);

        // Find matching frame array
        bool found_frames = false;
        for (const auto &arr : anim_frame_arrays_result.value()) {
            // Skip individual frame arrays (gTilesetAnims_..._Frame0), we want the main pointer array
            if (arr.name().find("_Frame") != std::string::npos) {
                continue;
            }

            std::string arr_anim_name =
                extract_anim_name_from_array_ref(arr.name(), pascal_case_tileset, porytiles_managed);

            // Case-insensitive comparison to handle inconsistencies like TVTurnedOn vs TvTurnedOn
            if (to_lower_str(arr_anim_name) == to_lower_str(pascal_name)) {
                auto frame_order = extract_frame_names(arr.elements());
                if (!frame_order.empty()) {
                    // Derive unique frame_names from frame_order (preserving first occurrence order)
                    std::vector<std::string> frame_names;
                    std::set<std::string> seen;
                    for (const auto &frame : frame_order) {
                        if (!seen.contains(frame)) {
                            seen.insert(frame);
                            frame_names.push_back(frame);
                        }
                    }
                    params.frame_names(std::move(frame_names));
                    params.frame_order(std::move(frame_order));
                    found_frames = true;
                }
                break;
            }
        }

        if (!found_frames) {
            diag_->warning(
                anim_parsing_error,
                format_->format(
                    "could not find frame array for animation '{}'",
                    FormatParam{to_snake_case(pascal_name), Style::bold}));
        }

        // Convert PascalCase to snake_case for result key
        const std::string snake_case_name = to_snake_case(pascal_name);
        result[snake_case_name] = std::move(params);
    }

    return result;
}

} // namespace porytiles2
