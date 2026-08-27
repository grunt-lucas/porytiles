#include "porytiles/infra/services/anim_code_parser.hpp"

#include <algorithm>
#include <format>
#include <map>
#include <optional>
#include <set>
#include <string>

#include "porytiles/domain/models/anim_params.hpp"
#include "porytiles/domain/models/animation.hpp"
#include "porytiles/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles/utilities/c_parser/function_call_info.hpp"
#include "porytiles/utilities/c_parser/token.hpp"
#include "porytiles/utilities/dynamic_cased_name.hpp"
#include "porytiles/utilities/text/file_highlight_printer.hpp"

namespace {

using namespace porytiles;

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

/// @brief Diagnostic tag for remarks emitted while parsing animation code.
constexpr const char *anim_code_parse_tag = "anim-code-parse";

/// @brief Candidate identifier prefixes for resolving animation array references.
///
/// @details
/// Strict candidates use the tileset's full shorthand, the expected naming convention. Fallback
/// candidates use shortened forms of the shorthand (trailing underscore-separated segments dropped one at a time). E.g.
/// shorthand "General_Frlg" might show up as "General" without the "_Frlg" suffix. See Expansion's frlg maps. Resolving
/// through a fallback candidate succeeds but is reported to the user via a remark.
struct AnimArrayPrefixCandidates {
    std::vector<std::string> strict;
    std::vector<std::string> fallback;
};

/// @brief Builds the candidate prefixes used to resolve an animation array identifier for a tileset.
///
/// @details
/// Porytiles-managed tilesets get a single strict candidate matching the naming emitted by AnimCodeGenerator
/// (gTilesetAnims_PorytilesManaged_{PascalTileset}_). Vanilla tilesets get strict candidates with both the g and s
/// prefixes, in both C identifier form ("General_Frlg") and PascalCase form ("GeneralFrlg"). Vanilla tilesets also
/// get fallback candidates with trailing shorthand segments dropped. Some Expansion tilesets name a tileset's
/// animation arrays with a truncated tileset shorthand (e.g. gTileset_General_Frlg names its arrays
/// sTilesetAnims_General_*).
///
/// @param tileset_cased_name The tileset shorthand as a DynamicCasedName
/// @param porytiles_managed Whether this uses the PorytilesManaged_ prefix
/// @return The strict and fallback candidate prefixes, each ending with an underscore
[[nodiscard]] AnimArrayPrefixCandidates
build_anim_array_prefix_candidates(const DynamicCasedName &tileset_cased_name, bool porytiles_managed)
{
    AnimArrayPrefixCandidates candidates;

    if (porytiles_managed) {
        candidates.strict.push_back(
            anim::g_tileset_anims_prefix + anim::porytiles_managed_prefix + tileset_cased_name.to_pascal_case() + "_");
        return candidates;
    }

    const std::string c_identifier = tileset_cased_name.to_c_identifier();
    candidates.strict.push_back(anim::g_tileset_anims_prefix + c_identifier + "_");
    candidates.strict.push_back(anim::s_tileset_anims_prefix + c_identifier + "_");

    const std::string pascal = tileset_cased_name.to_pascal_case();
    if (pascal != c_identifier) {
        candidates.strict.push_back(anim::g_tileset_anims_prefix + pascal + "_");
        candidates.strict.push_back(anim::s_tileset_anims_prefix + pascal + "_");
    }

    std::string shortened = c_identifier;
    for (auto underscore_pos = shortened.rfind('_'); underscore_pos != std::string::npos;
         underscore_pos = shortened.rfind('_')) {
        shortened = shortened.substr(0, underscore_pos);
        candidates.fallback.push_back(anim::g_tileset_anims_prefix + shortened + "_");
        candidates.fallback.push_back(anim::s_tileset_anims_prefix + shortened + "_");
    }

    return candidates;
}

/// @brief The outcome of resolving an animation array identifier against a tileset's candidate prefixes.
struct ResolvedAnimArrayRef {
    DynamicCasedName anim_name;
    std::string matched_prefix;
    bool used_shorthand_fallback{};
};

/// @brief Resolves the animation name from a gTilesetAnims/sTilesetAnims array reference.
///
/// @details
/// Tries the strict candidate prefixes first, then the shortened-shorthand fallbacks (see
/// build_anim_array_prefix_candidates). The remainder after the matched prefix, with any _Frame or _VDests suffix
/// trimmed, becomes the animation name. When no candidate matches, returns a multi-line error listing every prefix
/// that was tried.
///
/// @param identifier The full identifier name (from the first AppendTilesetAnimToBuffer argument)
/// @param tileset_cased_name The tileset shorthand as a DynamicCasedName
/// @param porytiles_managed Whether this uses the PorytilesManaged_ prefix
/// @param format Text formatter for error message styling
/// @return The resolved animation name and matched prefix, or an error if no candidate prefix matches
[[nodiscard]] ChainableResult<ResolvedAnimArrayRef> resolve_anim_name_from_array_ref(
    const std::string &identifier,
    const DynamicCasedName &tileset_cased_name,
    bool porytiles_managed,
    const TextFormatter &format)
{
    const auto candidates = build_anim_array_prefix_candidates(tileset_cased_name, porytiles_managed);

    auto try_prefixes = [&](const std::vector<std::string> &prefixes,
                            bool is_fallback) -> std::optional<ResolvedAnimArrayRef> {
        for (const auto &prefix : prefixes) {
            if (!identifier.starts_with(prefix)) {
                continue;
            }

            std::string remainder = identifier.substr(prefix.size());

            // Trim the _Frame suffix (individual frame arrays) or the _VDests suffix
            if (auto frame_pos = remainder.find("_Frame"); frame_pos != std::string::npos) {
                remainder = remainder.substr(0, frame_pos);
            }
            else if (auto vdests_pos = remainder.find("_VDests"); vdests_pos != std::string::npos) {
                remainder = remainder.substr(0, vdests_pos);
            }

            if (remainder.empty()) {
                continue;
            }

            return ResolvedAnimArrayRef{DynamicCasedName::from_c_identifier(remainder), prefix, is_fallback};
        }
        return std::nullopt;
    };

    if (auto resolved = try_prefixes(candidates.strict, false); resolved.has_value()) {
        return std::move(resolved).value();
    }
    if (auto resolved = try_prefixes(candidates.fallback, true); resolved.has_value()) {
        return std::move(resolved).value();
    }

    auto quote_join = [&format](const std::vector<std::string> &prefixes) {
        std::string joined;
        for (const auto &prefix : prefixes) {
            if (!joined.empty()) {
                joined += ", ";
            }
            joined += format.format("'{}'", FormatParam{prefix, Style::bold});
        }
        return joined;
    };

    std::vector<std::string> lines;
    lines.push_back(format.format("Could not extract animation name from '{}'.", FormatParam{identifier, Style::bold}));
    lines.push_back(
        format.format("Expected the array name to start with one of: {}.", FormatParam{quote_join(candidates.strict)}));
    if (!candidates.fallback.empty()) {
        lines.push_back(format.format(
            "Also tried shortened tileset shorthand prefixes: {}.", FormatParam{quote_join(candidates.fallback)}));
    }
    lines.push_back(format.format(
        "This usually means the animation arrays are named with a tileset shorthand that does not match '{}'.",
        FormatParam{tileset_cased_name.to_c_identifier(), Style::bold}));
    return FormattableError{lines};
}

/// @brief Extracts frame names from array element names.
///
/// @details
/// Given element names like ["..._Frame0", "..._Frame1", "..._Frame0", "..._Frame2"],
/// returns the frame names as DynamicCasedName instances ["0", "1", "0", "2"].
/// Frame names extracted from C identifiers (e.g., "0", "Center") are PascalCase.
///
/// @param elements Vector of element identifier names
/// @return Vector of frame names in order (as DynamicCasedName)
[[nodiscard]] std::vector<DynamicCasedName> extract_frame_names(const std::vector<std::string> &elements)
{
    std::vector<DynamicCasedName> frames;
    frames.reserve(elements.size());

    for (const auto &elem : elements) {
        // Find "_Frame" suffix and extract the frame name
        auto frame_pos = elem.find("_Frame");
        if (frame_pos != std::string::npos) {
            std::string frame_str = elem.substr(frame_pos + 6); // Skip "_Frame"
            frames.push_back(DynamicCasedName::from_pascal_case(frame_str));
        }
    }

    return frames;
}

/// @brief Finds TILE_OFFSET_4BPP(X) in tokens and returns X.
///
/// @details
/// Searches for the token pattern: identifier("TILE_OFFSET_4BPP") + lparen + integer + rparen.
/// On failure, produces a multi-line error showing the expected pattern vs the actual input tokens.
///
/// @param tokens The token sequence to search
/// @param format Text formatter for error message styling
/// @return The tile offset value, or a descriptive error if the pattern was not found
[[nodiscard]] ChainableResult<std::size_t>
extract_tile_offset(const std::vector<Token> &tokens, const TextFormatter &format)
{
    for (std::size_t i = 0; i + 3 < tokens.size(); ++i) {
        // Primary pattern: TILE_OFFSET_4BPP(<integer>)
        if (tokens[i].is(TokenType::identifier) && tokens[i].text() == "TILE_OFFSET_4BPP" &&
            tokens[i + 1].is(TokenType::left_paren) && tokens[i + 2].is(TokenType::integer_literal) &&
            tokens[i + 3].is(TokenType::right_paren)) {
            return tokens[i + 2].int_value();
        }

        // Secondary pattern: TILE_OFFSET_4BPP(NUM_TILES_IN_PRIMARY + <integer>)
        if (i + 5 < tokens.size() && tokens[i].is(TokenType::identifier) && tokens[i].text() == "TILE_OFFSET_4BPP" &&
            tokens[i + 1].is(TokenType::left_paren) && tokens[i + 2].is(TokenType::identifier) &&
            tokens[i + 2].text() == "NUM_TILES_IN_PRIMARY" && tokens[i + 3].is(TokenType::plus) &&
            tokens[i + 4].is(TokenType::integer_literal) && tokens[i + 5].is(TokenType::right_paren)) {
            return tokens[i + 4].int_value();
        }
    }

    std::string actual;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) {
            actual += " ";
        }
        actual += tokens[i].text();
    }

    return FormattableError{std::vector<std::string>{
        format.format(
            "Expected token pattern containing '{}' or '{}'.",
            FormatParam{"TILE_OFFSET_4BPP(<integer>)", Style::bold},
            FormatParam{"TILE_OFFSET_4BPP(NUM_TILES_IN_PRIMARY + <integer>)", Style::bold}),
        format.format("Actual tokens: '{}'.", FormatParam{actual, Style::bold}),
    }};
}

/// @brief Finds X * TILE_SIZE_4BPP in tokens and returns X.
///
/// @details
/// Searches for the token pattern: integer + star + identifier("TILE_SIZE_4BPP").
/// On failure, produces a multi-line error showing the expected pattern vs the actual input tokens.
///
/// @param tokens The token sequence to search
/// @param format Text formatter for error message styling
/// @return The tile count value, or a descriptive error if the pattern was not found
[[nodiscard]] ChainableResult<std::size_t>
extract_tile_count(const std::vector<Token> &tokens, const TextFormatter &format)
{
    for (std::size_t i = 0; i + 2 < tokens.size(); ++i) {
        if (tokens[i].is(TokenType::integer_literal) && tokens[i + 1].is(TokenType::star) &&
            tokens[i + 2].is(TokenType::identifier) && tokens[i + 2].text() == "TILE_SIZE_4BPP") {
            return tokens[i].int_value();
        }
    }

    std::string actual;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) {
            actual += " ";
        }
        actual += tokens[i].text();
    }

    return FormattableError{std::vector<std::string>{
        format.format(
            "Expected token pattern containing '{}'.",
            FormatParam{"<tile_count_integer> * TILE_SIZE_4BPP", Style::bold}),
        format.format("Actual tokens: '{}'.", FormatParam{actual, Style::bold}),
    }};
}

/// @brief Finds the driver function name from an Init callback function body.
///
/// @details
/// Searches for assignment pattern: sPrimaryTilesetAnimCallback = <driver_func>
/// or: sSecondaryTilesetAnimCallback = <driver_func>
///
/// @param body_tokens The callback function body tokens
/// @return The driver function name, or an error if the callback assignment pattern was not found
[[nodiscard]] ChainableResult<std::string> find_driver_function_from_callback(const std::vector<Token> &body_tokens)
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
    return FormattableError{"Could not find tileset anim callback assignment in function body."};
}

/// @brief Represents a discovered timer condition and its associated function call.
struct TimerCondition {
    std::size_t frame_factor; // The X in timer % X
    std::size_t frame_offset; // The Y in timer % X == Y
    std::string called_func;  // The function called inside the condition block
};

/// @brief Extracts timer conditions and associated function calls from a driver function.
///
/// @details
/// Searches for patterns like: if (timer % X == Y) { ... func_call(...) ... }
/// Returns a vector of TimerCondition structs.
///
/// @param body_tokens The driver function body tokens
/// @return Vector of timer conditions with their associated function calls
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

/// @brief Represents animation data discovered from AppendTilesetAnimToBuffer calls.
struct DiscoveredAnimData {
    std::string array_identifier; // Frame pointer array identifier referenced by the queue function
    std::size_t tile_offset{};
    std::size_t tile_count{};
    std::size_t frame_factor{};
    std::size_t frame_offset{};
};

/// @brief Extracts animation name from the first argument of AppendTilesetAnimToBuffer.
///
/// @details
/// The first argument is typically something like: gTilesetAnims_General_Flower[i]
/// We need to extract the identifier before the array subscript.
///
/// @param arg_tokens The tokens of the first argument
/// @return The identifier name, or an error if no identifier was found in the argument tokens
[[nodiscard]] ChainableResult<std::string> extract_array_name_from_first_arg(const std::vector<Token> &arg_tokens)
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

    return FormattableError{"No identifier found in first argument of AppendTilesetAnimToBuffer call."};
}

/// @brief Holds parsed function definitions and a name-to-pointer lookup map.
///
/// @details
/// The @c by_name map stores raw pointers into the @c definitions vector. Moving a @c std::vector transfers the
/// internal buffer pointer, so element addresses remain stable. Callers must keep the ParsedFunctions instance alive
/// while using
/// @c by_name pointers.
///
/// @invariant All pointers in @c by_name remain valid as long as @c definitions is not modified.
struct ParsedFunctions {
    std::vector<FunctionDefinition> definitions;
    std::map<std::string, const FunctionDefinition *> by_name;
};

/// @brief Step 1: Parses the callback function to find the driver function name.
///
/// @details
/// Parses the callback function (e.g., InitTilesetAnim_General) and searches for a driver function assignment
/// (sPrimaryTilesetAnimCallback = ...) in the body. Returns the driver function name if found, or an empty string if
/// no callback function exists (no animations to process).
///
/// @param c_parser The parser facade for the C file
/// @param callback_func_name The callback function name to search for
/// @param c_file_path Path to the C file (for error messages)
/// @param format Text formatter for error message styling
/// @return The driver function name, an empty string if no callback found, or an error
[[nodiscard]] ChainableResult<std::string> step_1_find_driver_function(
    CParserFacade &c_parser,
    const std::string &callback_func_name,
    const std::filesystem::path &c_file_path,
    const TextFormatter *format)
{
    auto callback_funcs_result = c_parser.parse_functions(callback_func_name);
    if (!callback_funcs_result.has_value()) {
        return ChainableResult<std::string>{
            FormattableError{format->format(
                "'{}': Failed to parse callback function.", FormatParam{c_file_path.string(), Style::bold})},
            callback_funcs_result};
    }

    auto &callback_funcs = callback_funcs_result.value();
    // Narrow from prefix match to exact name match (parse_functions uses starts_with)
    std::erase_if(callback_funcs, [&](const FunctionDefinition &func) { return func.name() != callback_func_name; });
    if (callback_funcs.empty()) {
        return std::string{};
    }

    if (callback_funcs.size() > 1) {
        return FormattableError{
            "Found multiple callback functions matching '{}'.", FormatParam{callback_func_name, Style::bold}};
    }

    const auto &callback_func = callback_funcs.front();
    auto driver_func_name_result = find_driver_function_from_callback(callback_func.body_tokens());
    if (!driver_func_name_result.has_value()) {
        return ChainableResult<std::string>{
            FormattableError{
                "Could not find driver function assignment in '{}'.", FormatParam{callback_func_name, Style::bold}},
            driver_func_name_result};
    }

    return std::move(driver_func_name_result).value();
}

/// @brief Step 2: Parses the driver function and extracts timer conditions.
///
/// @details
/// Parses the driver function (e.g., TilesetAnim_General) and extracts timer conditions of the form
/// @c if @c (timer @c % @c X @c == @c Y) along with associated function calls.
///
/// @param c_parser The parser facade for the C file
/// @param driver_func_name The driver function name to parse
/// @param c_file_path Path to the C file (for error messages)
/// @param format Text formatter for error message styling
/// @return Timer conditions, or an error if the driver function is missing or has no timer conditions
[[nodiscard]] ChainableResult<std::vector<TimerCondition>> step_2_extract_timer_conditions(
    CParserFacade &c_parser,
    const std::string &driver_func_name,
    const std::filesystem::path &c_file_path,
    const TextFormatter *format)
{
    auto driver_funcs_result = c_parser.parse_functions(driver_func_name);
    if (!driver_funcs_result.has_value()) {
        return ChainableResult<std::vector<TimerCondition>>{
            FormattableError{format->format(
                "'{}': Failed to parse driver function '{}'.",
                FormatParam{c_file_path.string(), Style::bold},
                FormatParam{driver_func_name, Style::bold})},
            driver_funcs_result};
    }

    auto &driver_funcs = driver_funcs_result.value();
    // Narrow from prefix match to exact name match (parse_functions uses starts_with)
    std::erase_if(driver_funcs, [&](const FunctionDefinition &func) { return func.name() != driver_func_name; });
    if (driver_funcs.empty()) {
        return FormattableError{"Driver function '{}' not found in file.", FormatParam{driver_func_name, Style::bold}};
    }

    const auto &driver_func = driver_funcs.front();
    std::vector<TimerCondition> timer_conditions = extract_timer_conditions(driver_func.body_tokens());

    if (timer_conditions.empty()) {
        return FormattableError{
            "No timer conditions found in driver function '{}'.", FormatParam{driver_func_name, Style::bold}};
    }

    return timer_conditions;
}

/// @brief Step 3: Parses all functions in the file and builds a name-to-pointer lookup map.
///
/// @details
/// Parses every function definition in the C file and builds a map for O(1) lookup by name. The returned
/// ParsedFunctions struct owns the definitions vector; the @c by_name map stores raw pointers into it.
///
/// @param c_parser The parser facade for the C file
/// @param c_file_path Path to the C file (for error messages)
/// @param format Text formatter for error message styling
/// @return ParsedFunctions with definitions and lookup map
[[nodiscard]] ChainableResult<ParsedFunctions> step_3_build_function_map(
    CParserFacade &c_parser, const std::filesystem::path &c_file_path, const TextFormatter *format)
{
    auto all_funcs_result = c_parser.parse_functions();
    if (!all_funcs_result.has_value()) {
        return ChainableResult<ParsedFunctions>{
            FormattableError{
                format->format("'{}': Failed to parse functions.", FormatParam{c_file_path.string(), Style::bold})},
            all_funcs_result};
    }

    ParsedFunctions parsed;
    parsed.definitions = std::move(all_funcs_result).value();
    for (const auto &func : parsed.definitions) {
        parsed.by_name[func.name()] = &func;
    }

    return parsed;
}

/// @brief Step 4: Extracts animation data from queue functions found via timer conditions.
///
/// @details
/// For each timer condition, looks up the called queue function in the function map, finds
/// @c AppendTilesetAnimToBuffer calls, and extracts animation name, tile offset, and tile count.
///
/// @param timer_conditions Timer conditions from the driver function
/// @param func_map Function name to definition pointer map
/// @param tileset_cased_name The tileset shorthand as a DynamicCasedName
/// @param porytiles_managed Whether this uses the PorytilesManaged_ prefix
/// @param format Text formatter for error message styling
/// @param diag UserDiagnostics for emitting a remark when a shortened shorthand fallback resolves an array name
/// @return Map of DynamicCasedName animation name to discovered animation data, or an error
[[nodiscard]] ChainableResult<std::map<DynamicCasedName, DiscoveredAnimData>> step_4_extract_animation_data(
    const std::vector<TimerCondition> &timer_conditions,
    const std::map<std::string, const FunctionDefinition *> &func_map,
    const DynamicCasedName &tileset_cased_name,
    bool porytiles_managed,
    const TextFormatter *format,
    const UserDiagnostics *diag)
{
    std::map<DynamicCasedName, DiscoveredAnimData> discovered_anims;

    for (const auto &condition : timer_conditions) {
        auto it = func_map.find(condition.called_func);
        if (it == func_map.end()) {
            return FormattableError{
                "Queue function '{}' not found in file.", FormatParam{condition.called_func, Style::bold}};
        }

        const FunctionDefinition *queue_func = it->second;

        // Find AppendTilesetAnimToBuffer calls in the queue function
        auto append_calls = find_function_calls(queue_func->body_tokens(), "AppendTilesetAnimToBuffer");

        if (append_calls.empty()) {
            return FormattableError{
                "No AppendTilesetAnimToBuffer calls in queue function '{}'.",
                FormatParam{condition.called_func, Style::bold}};
        }

        // Process the first AppendTilesetAnimToBuffer call
        // Note: For VDests patterns, there may be multiple calls - we defer full handling per design decision
        const auto &call = append_calls.front();

        if (call.argument_count() < 3) {
            return FormattableError{
                "AppendTilesetAnimToBuffer call in '{}' has fewer than 3 arguments.",
                FormatParam{condition.called_func, Style::bold}};
        }

        // Extract animation name from first argument (e.g., gTilesetAnims_General_Flower[i])
        auto array_name_result = extract_array_name_from_first_arg(call.argument_at(0));
        if (!array_name_result.has_value()) {
            return ChainableResult<std::map<DynamicCasedName, DiscoveredAnimData>>{
                FormattableError{
                    "Failed to parse animation data from queue function '{}'.",
                    FormatParam{condition.called_func, Style::bold}},
                array_name_result};
        }

        auto resolved_result =
            resolve_anim_name_from_array_ref(array_name_result.value(), tileset_cased_name, porytiles_managed, *format);
        if (!resolved_result.has_value()) {
            return ChainableResult<std::map<DynamicCasedName, DiscoveredAnimData>>{
                FormattableError{
                    "Failed to parse animation data from queue function '{}'.",
                    FormatParam{condition.called_func, Style::bold}},
                resolved_result};
        }
        auto resolved = std::move(resolved_result).value();

        if (resolved.used_shorthand_fallback) {
            diag->remark(
                anim_code_parse_tag,
                std::vector<std::string>{
                    format->format(
                        "Animation array '{}' is not named with the tileset shorthand '{}'.",
                        FormatParam{array_name_result.value(), Style::bold},
                        FormatParam{tileset_cased_name.to_c_identifier(), Style::bold}),
                    format->format(
                        "Resolved animation '{}' using the shortened shorthand prefix '{}'.",
                        FormatParam{resolved.anim_name.to_snake_case(), Style::bold},
                        FormatParam{resolved.matched_prefix, Style::bold}),
                });
        }

        // Extract tile_offset from second argument
        auto tile_offset = extract_tile_offset(call.argument_at(1), *format);
        if (!tile_offset.has_value()) {
            return ChainableResult<std::map<DynamicCasedName, DiscoveredAnimData>>{
                FormattableError{std::vector{
                    format->format(
                        "Failed to extract '{}' from second argument of '{}' call in '{}'.",
                        FormatParam{"TILE_OFFSET_4BPP", Style::bold},
                        FormatParam{"AppendTilesetAnimToBuffer", Style::bold},
                        FormatParam{condition.called_func, Style::bold}),
                    format->format("Full call: '{}'.", FormatParam{call.reconstruct_call_text(), Style::bold}),
                }},
                tile_offset};
        }

        // Extract tile_count from third argument
        auto tile_count = extract_tile_count(call.argument_at(2), *format);
        if (!tile_count.has_value()) {
            return ChainableResult<std::map<DynamicCasedName, DiscoveredAnimData>>{
                FormattableError{std::vector{
                    format->format(
                        "Failed to extract '{}' from third argument of '{}' call in '{}'.",
                        FormatParam{"TILE_SIZE_4BPP", Style::bold},
                        FormatParam{"AppendTilesetAnimToBuffer", Style::bold},
                        FormatParam{condition.called_func, Style::bold}),
                    format->format("Full call: '{}'.", FormatParam{call.reconstruct_call_text(), Style::bold}),
                }},
                tile_count};
        }

        if (append_calls.size() > 1) {
            return FormattableError{
                "Queue function '{}' has multiple AppendTilesetAnimToBuffer calls (VDests pattern not yet supported).",
                FormatParam{condition.called_func, Style::bold}};
        }

        // Store discovered animation data
        discovered_anims[resolved.anim_name] = {
            array_name_result.value(),
            tile_offset.value(),
            tile_count.value(),
            condition.frame_factor,
            condition.frame_offset};
    }

    return discovered_anims;
}

/// @brief Step 5: Parses all pointer array declarations from the file.
///
/// @details
/// Parses every pointer array declaration in the C file. Step 6 selects the arrays it needs by exact identifier
/// match against the array names recorded in step 4, so no name-prefix filtering happens here.
///
/// @param c_parser The parser facade for the C file
/// @param c_file_path Path to the C file (for error messages)
/// @param format Text formatter for error message styling
/// @return Vector of all pointer array declarations in the file
[[nodiscard]] ChainableResult<std::vector<ArrayDeclaration>> step_5_parse_frame_arrays(
    CParserFacade &c_parser, const std::filesystem::path &c_file_path, const TextFormatter *format)
{
    auto anim_frame_arrays_result = c_parser.parse_pointer_arrays();
    if (!anim_frame_arrays_result.has_value()) {
        return ChainableResult<std::vector<ArrayDeclaration>>{
            FormattableError{format->format(
                "{}: Failed to parse animation frame arrays.", FormatParam{c_file_path.string(), Style::bold})},
            anim_frame_arrays_result};
    }

    return anim_frame_arrays_result;
}

/// @brief Step 6: Matches discovered animations with frame arrays and builds the final AnimParams map.
///
/// @details
/// For each discovered animation, finds its frame pointer array by exact identifier match, extracts frame names and
/// order, and constructs an AnimParams. Returns an error if the referenced array is missing or has no _Frame
/// elements.
///
/// @param discovered_anims Map of DynamicCasedName animation name to discovered animation data
/// @param frame_arrays Vector of parsed frame pointer array declarations
/// @param format Text formatter for error message styling
/// @return Map of DynamicCasedName to AnimParams, or error if a frame array is missing or empty
[[nodiscard]] ChainableResult<std::map<DynamicCasedName, AnimParams>> step_6_build_animation_params(
    const std::map<DynamicCasedName, DiscoveredAnimData> &discovered_anims,
    const std::vector<ArrayDeclaration> &frame_arrays,
    const TextFormatter *format)
{
    std::map<DynamicCasedName, AnimParams> result;

    for (const auto &[cased_name, anim_data] : discovered_anims) {
        AnimParams params;
        params.tile_offset(anim_data.tile_offset);
        params.tile_count(anim_data.tile_count);
        params.frame_factor(anim_data.frame_factor);
        params.frame_offset(anim_data.frame_offset);

        // The queue function referenced this array by name, so an exact identifier match is the correct lookup
        const auto array_it = std::ranges::find_if(
            frame_arrays, [&](const ArrayDeclaration &arr) { return arr.name() == anim_data.array_identifier; });
        if (array_it == frame_arrays.end()) {
            return FormattableError{std::vector<std::string>{
                format->format(
                    "Could not find frame array '{}' for animation '{}'.",
                    FormatParam{anim_data.array_identifier, Style::bold},
                    FormatParam{cased_name.to_snake_case(), Style::bold}),
                "A queue function references this array, but the file contains no pointer array declaration with "
                "that name.",
            }};
        }

        auto frame_order = extract_frame_names(array_it->elements());
        if (frame_order.empty()) {
            return FormattableError{
                "Frame array '{}' for animation '{}' has no elements with a '{}' suffix.",
                FormatParam{anim_data.array_identifier, Style::bold},
                FormatParam{cased_name.to_snake_case(), Style::bold},
                FormatParam{"_Frame", Style::bold}};
        }

        // Derive unique frame_names from frame_order (preserving first occurrence order)
        std::vector<DynamicCasedName> frame_names;
        std::set<DynamicCasedName> seen;
        for (const auto &frame : frame_order) {
            if (!seen.contains(frame)) {
                seen.insert(frame);
                frame_names.push_back(frame);
            }
        }
        params.frame_names(std::move(frame_names));
        params.frame_order(std::move(frame_order));
        params.cased_name(cased_name);
        params.frame_array_identifier(anim_data.array_identifier);

        result[cased_name] = std::move(params);
    }

    return result;
}

} // namespace

namespace porytiles {

ChainableResult<std::map<DynamicCasedName, AnimParams>> AnimCodeParser::parse_from_callback(
    const std::filesystem::path &c_file_path,
    const std::string &callback_func_name,
    const DynamicCasedName &tileset_cased_name,
    bool porytiles_managed) const
{
    CParserFacade c_parser{c_file_path, format_};

    using ResultType = std::map<DynamicCasedName, AnimParams>;

    // Step 1: Parse callback function -> find driver function name
    PT_TRY_ASSIGN_PASS_ERR(
        driver_func_name, step_1_find_driver_function(c_parser, callback_func_name, c_file_path, format_), ResultType);
    if (driver_func_name.empty()) {
        return ResultType{};
    }

    // Step 2: Parse driver function -> extract timer conditions
    PT_TRY_ASSIGN_PASS_ERR(
        timer_conditions,
        step_2_extract_timer_conditions(c_parser, driver_func_name, c_file_path, format_),
        ResultType);

    // Step 3: Parse all functions -> build lookup map
    PT_TRY_ASSIGN_PASS_ERR(parsed_funcs, step_3_build_function_map(c_parser, c_file_path, format_), ResultType);

    // Step 4: Extract animation data from queue functions
    PT_TRY_ASSIGN_PASS_ERR(
        discovered_anims,
        step_4_extract_animation_data(
            timer_conditions, parsed_funcs.by_name, tileset_cased_name, porytiles_managed, format_, diag_),
        ResultType);

    // Step 5: Parse frame pointer arrays
    PT_TRY_ASSIGN_PASS_ERR(frame_arrays, step_5_parse_frame_arrays(c_parser, c_file_path, format_), ResultType);

    // Step 6: Build final AnimParams map
    return step_6_build_animation_params(discovered_anims, frame_arrays, format_);
}

} // namespace porytiles
