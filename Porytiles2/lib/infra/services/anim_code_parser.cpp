#include "porytiles2/infra/services/anim_code_parser.hpp"

#include <map>
#include <optional>
#include <set>
#include <string>

#include "fmt/format.h"

#include "porytiles2/domain/models/animation_params.hpp"
#include "porytiles2/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles2/utilities/c_parser/token.hpp"
#include "porytiles2/utilities/functional/transform.hpp"
#include "porytiles2/utilities/string_utils.hpp"
#include "porytiles2/utilities/text/file_highlight_printer.hpp"

namespace {

using namespace porytiles2;

constexpr std::string porytiles_managed_prefix = "PorytilesManaged_";

constexpr auto anim_parsing_error = "animation-parsing-error";

/**
 * @brief Extracts the PascalCase animation name from an identifier.
 *
 * @details
 * For Porytiles-managed format: "gTilesetAnims_PorytilesManaged_General_Flower" -> "Flower"
 * For vanilla format: "gTilesetAnims_General_Flower" -> "Flower"
 *
 * @param identifier The full identifier name
 * @param pascal_case_tileset_name The tileset name to help locate the animation name portion
 * @return The animation name, or empty string if not found
 */
[[nodiscard]] std::string extract_anim_name(const std::string &identifier, const std::string &pascal_case_tileset_name)
{
    // Try Porytiles-managed format: gTilesetAnims_PorytilesManaged_TilesetName_AnimName
    std::string porytiles_prefix = "gTilesetAnims_" + porytiles_managed_prefix + pascal_case_tileset_name + "_";
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
    std::string vanilla_prefix = "gTilesetAnims_" + pascal_case_tileset_name + "_";
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
 * @param pascal_case_tileset_name The tileset name to help locate the animation name portion
 * @return The animation name, or empty string if not a matching function
 */
[[nodiscard]] std::string
extract_anim_name_from_function(const std::string &func_name, const std::string &pascal_case_tileset_name)
{
    // Try Porytiles-managed format: QueueAnimTiles_PorytilesManaged_TilesetName_AnimName
    std::string porytiles_prefix = "QueueAnimTiles_" + porytiles_managed_prefix + pascal_case_tileset_name + "_";
    if (func_name.starts_with(porytiles_prefix)) {
        return func_name.substr(porytiles_prefix.size());
    }

    // Try vanilla format: QueueAnimTiles_TilesetName_AnimName
    std::string vanilla_prefix = "QueueAnimTiles_" + pascal_case_tileset_name + "_";
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
            return body_tokens[i + 2].int_value();
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
            return body_tokens[i].int_value();
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
 * @param pascal_case_tileset_name The tileset name for identifying animation calls
 * @return Map of animation names to their (frame_factor, frame_offset) pairs
 */
[[nodiscard]] std::map<std::string, std::pair<std::size_t, std::size_t>>
extract_timer_conditions(const std::vector<Token> &body_tokens, const std::string &pascal_case_tileset_name)
{
    std::map<std::string, std::pair<std::size_t, std::size_t>> result;

    // Look for: timer % X == Y patterns followed by QueueAnimTiles calls
    for (std::size_t i = 0; i + 6 < body_tokens.size(); ++i) {
        // Check for: timer % X == Y
        if (body_tokens[i].is(TokenType::identifier) && body_tokens[i].text() == "timer" &&
            body_tokens[i + 1].is(TokenType::percent) && body_tokens[i + 2].is(TokenType::integer_literal) &&
            body_tokens[i + 3].is(TokenType::equal_equal) && body_tokens[i + 4].is(TokenType::integer_literal)) {

            std::size_t frame_factor = body_tokens[i + 2].int_value();
            std::size_t frame_offset = body_tokens[i + 4].int_value();

            // Search ahead for QueueAnimTiles call
            for (std::size_t j = i + 5; j < body_tokens.size() && j < i + 50; ++j) {
                if (body_tokens[j].is(TokenType::identifier) && body_tokens[j].text().starts_with("QueueAnimTiles_")) {
                    std::string anim_name =
                        extract_anim_name_from_function(body_tokens[j].text(), pascal_case_tileset_name);
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

[[nodiscard]] std::vector<std::string> make_highlighted_details(
    const SourcePosition &position,
    const TextFormatter &format,
    const std::filesystem::path &file_path,
    const std::string &message)
{
    const FileHighlightPrinter printer{&format};

    // Build error details
    std::vector<std::string> details;
    details.push_back(fmt::format("{}:{}:{}: {}", file_path.string(), position.line, position.column, message));
    details.emplace_back();

    // Add source context if position is valid
    if (position.line > 0) {
        auto context = printer.print(file_path, position.line - 1, position.column - 1);
        details.insert(details.end(), context.begin(), context.end());
    }

    return details;
}

ChainableResult<std::map<std::string, AnimationParams>> parse_animation_params_from_c_file(
    const std::filesystem::path &c_file,
    const std::string &tileset_name,
    const std::set<std::string> &expected_anim_names,
    bool porytiles_managed,
    const TextFormatter &format,
    const UserDiagnostics &diag)
{
    std::map<std::string, AnimationParams> result;
    CParserFacade c_parser{c_file, &format};
    const std::string pascal_case_tileset_name = to_pascal_case(tileset_name);

    /*
     * First step: try to parse the animation frame arrays, driver function, and QueueAnimTiles functions for this
     * tileset.
     */

    // Parse pointer arrays to get animation frame sequences
    const auto frame_array_prefix =
        "gTilesetAnims_" + (porytiles_managed ? porytiles_managed_prefix : std::string{}) + pascal_case_tileset_name;
    auto anim_frame_arrays_result = c_parser.parse_pointer_arrays(frame_array_prefix);
    if (!anim_frame_arrays_result.has_value()) {
        return ChainableResult<std::map<std::string, AnimationParams>>{
            FormattableError{
                format.format("{}: failed to parse animation frame arrays", FormatParam{c_file.string(), Style::bold})},
            anim_frame_arrays_result};
    }
    const auto &anim_frame_arrays = anim_frame_arrays_result.value();

    // Parse functions to get driver function
    const auto driver_prefix =
        "TilesetAnim_" + (porytiles_managed ? porytiles_managed_prefix : std::string{}) + pascal_case_tileset_name;
    auto driver_funcs_result = c_parser.parse_functions(driver_prefix);
    if (!driver_funcs_result.has_value()) {
        return ChainableResult<std::map<std::string, AnimationParams>>{
            FormattableError{format.format(
                "{}: failed to parse animation driver functions", FormatParam{c_file.string(), Style::bold})},
            driver_funcs_result};
    }
    const auto &driver_funcs = driver_funcs_result.value();
    if (driver_funcs.size() > 1) {
        diag.err(
            "multiple-anim-driver-functions",
            format.format(
                "found multiple animation driver function candidates for '{}'",
                FormatParam{tileset_name, Style::bold}));
        for (const auto &driver_func : driver_funcs) {
            diag.note(
                "multiple-anim-driver-functions",
                make_highlighted_details(driver_func.position(), format, c_file, "found driver function candidate:"));
        }
        return FormattableError{
            "found multiple animation driver function candidates (prefix '{}')",
            FormatParam{driver_prefix, Style::bold}};
    }

    // Parse functions to get QueueAnimTiles functions
    const auto queue_prefix =
        "QueueAnimTiles_" + (porytiles_managed ? porytiles_managed_prefix : std::string{}) + pascal_case_tileset_name;
    auto queue_anim_funcs_result = c_parser.parse_functions(queue_prefix);
    if (!queue_anim_funcs_result.has_value()) {
        return ChainableResult<std::map<std::string, AnimationParams>>{
            FormattableError{format.format(
                "{}: failed to parse animation QueueAnimTiles functions", FormatParam{c_file.string(), Style::bold})},
            queue_anim_funcs_result};
    }
    const auto &queue_anim_funcs = queue_anim_funcs_result.value();

    /*
     * If all the following are empty:
     *
     * - anim_frame_arrays
     * - driver_funcs
     * - queue_anim_funcs
     *
     * then we can assume this tileset has no animations, and return an empty map.
     *
     * HOWEVER, if one of these arrays has values, but another doesn't, we throw an error that says "incomplete tileset
     * animation parameters, no animations will be parsed"
     */
    if (anim_frame_arrays.empty() && driver_funcs.empty() && queue_anim_funcs.empty()) {
        return std::map<std::string, AnimationParams>{};
    }
    if (anim_frame_arrays.empty() || driver_funcs.empty() || queue_anim_funcs.empty()) {
        constexpr auto incomplete_anim_params_tag = "incomplete-tileset-animation-parameters";
        if (anim_frame_arrays.empty()) {
            diag.err(
                incomplete_anim_params_tag,
                format.format(
                    "no animation frame pointer arrays exist for '{}'", FormatParam{tileset_name, Style::bold}));
            for (const auto &driver_func : driver_funcs) {
                diag.note(
                    incomplete_anim_params_tag,
                    make_highlighted_details(
                        driver_func.position(), format, c_file, "found driver function candidate:"));
            }
            for (const auto &queue_func : queue_anim_funcs) {
                diag.note(
                    incomplete_anim_params_tag,
                    make_highlighted_details(
                        queue_func.position(), format, c_file, "found QueueAnimTiles function candidate:"));
            }
        }
        if (driver_funcs.empty()) {
            diag.err(
                incomplete_anim_params_tag,
                format.format("no driver function exists for '{}'", FormatParam{tileset_name, Style::bold}));
            for (const auto &anim_frame_array : anim_frame_arrays) {
                diag.note(
                    incomplete_anim_params_tag,
                    make_highlighted_details(
                        anim_frame_array.position(), format, c_file, "found animation frame array candidate:"));
            }
            for (const auto &queue_func : queue_anim_funcs) {
                diag.note(
                    incomplete_anim_params_tag,
                    make_highlighted_details(
                        queue_func.position(), format, c_file, "found QueueAnimTiles function candidate:"));
            }
        }
        if (queue_anim_funcs.empty()) {
            diag.err(
                incomplete_anim_params_tag,
                format.format("no QueueAnimTiles functions exist for '{}'", FormatParam{tileset_name, Style::bold}));
            for (const auto &anim_frame_array : anim_frame_arrays) {
                diag.note(
                    incomplete_anim_params_tag,
                    make_highlighted_details(
                        anim_frame_array.position(), format, c_file, "found animation frame array candidate:"));
            }
            for (const auto &driver_func : driver_funcs) {
                diag.note(
                    incomplete_anim_params_tag,
                    make_highlighted_details(
                        driver_func.position(), format, c_file, "found driver function candidate:"));
            }
        }
        diag.err(
            incomplete_anim_params_tag,
            format.format("no animations were parsed for '{}'", FormatParam{tileset_name, Style::bold}));
        return FormattableError{
            "tileset '{}' specified animations with incomplete parameters", FormatParam{tileset_name, Style::bold}};
    }

    /*
     * Next, we parse the driver function to get timer conditions. We already validated that there is exactly one driver
     * function for this tileset.
     */
    const auto driver_func = driver_funcs.at(0);
    std::map<std::string, std::pair<std::size_t, std::size_t>> timer_conditions =
        extract_timer_conditions(driver_func.body_tokens(), pascal_case_tileset_name);

    /*
     * Finally, we can build AnimationParams for each animation.
     */
    for (const std::set<std::string> pascal_case_anim_names = transform(expected_anim_names, to_pascal_case);
         const auto &pascal_case_anim_name : pascal_case_anim_names) {
        AnimationParams params;

        // TODO: fail if one of the pascal_case_anim_names is missing from the file we're parsing

        // Find the frame sequence from the pointer array
        for (const auto &anim_frame_array : anim_frame_arrays) {
            /*
             * We want the main pointer array (e.g., gTilesetAnims_General_Flower), not the
             * individual frame data arrays (e.g., gTilesetAnims_General_Flower_Frame0).
             * Both extract to the same anim_name, so we filter by checking for _Frame.
             */
            if (const std::string arr_anim_name = extract_anim_name(anim_frame_array.name(), pascal_case_tileset_name);
                arr_anim_name == pascal_case_anim_name && anim_frame_array.name().find("_Frame") == std::string::npos) {
                if (auto frames = extract_frame_indices(anim_frame_array.elements()); !frames.empty()) {
                    params.frames(std::move(frames));
                }
                else {
                    return FormattableError{make_highlighted_details(
                        anim_frame_array.position(),
                        format,
                        c_file,
                        format.format(
                            "Failed to parse frame configuration from '{}'",
                            FormatParam{anim_frame_array.name(), Style::bold}))};
                }
                break;
            }
        }

        // Find tile_offset and tile_count from the QueueAnimTiles function
        bool found_queue_anim_func = false;
        for (const auto &queue_anim_func : queue_anim_funcs) {
            std::string func_anim_name =
                extract_anim_name_from_function(queue_anim_func.name(), pascal_case_tileset_name);

            /*
             * The to_lower_str comparison is needed here for building::tv_turned_on because the animation queue
             * function calls it "TVTurnedOn" but the frame array is called "TvTurnedOn" *facepalm*. There may be other
             * cases where users have followed different PascalCase conventions, so let's just try to accommodate all of
             * them. Note: Porytiles will need to enforce that animation "canonical" names, i.e. the names of the anim
             * folders, are snake_lower_case. This is a small ask for users who want to onboard to Porytiles.
             */
            if (to_lower_str(func_anim_name) == to_lower_str(pascal_case_anim_name)) {
                found_queue_anim_func = true;
                if (const auto tile_offset = extract_tile_offset(queue_anim_func.body_tokens());
                    tile_offset.has_value()) {
                    params.tile_offset(tile_offset.value());
                }
                else {
                    return FormattableError{make_highlighted_details(
                        queue_anim_func.position(),
                        format,
                        c_file,
                        format.format(
                            "QueueAnimTiles function '{}' missing TILE_OFFSET_4BPP call",
                            FormatParam{queue_anim_func.name(), Style::bold}))};
                }

                if (const auto tile_count = extract_tile_count(queue_anim_func.body_tokens()); tile_count.has_value()) {
                    params.tile_count(tile_count.value());
                }
                else {
                    return FormattableError{make_highlighted_details(
                        queue_anim_func.position(),
                        format,
                        c_file,
                        format.format(
                            "QueueAnimTiles function '{}' missing TILE_SIZE_4BPP expression",
                            FormatParam{queue_anim_func.name(), Style::bold}))};
                }
                break;
            }
        }

        if (!found_queue_anim_func) {
            return FormattableError{format.format(
                "failed to find QueueAnimTiles function for animation '{}'",
                FormatParam{to_snake_case(pascal_case_anim_name), Style::bold})};
        }

        // Get timer conditions from the driver function
        if (const auto it = timer_conditions.find(pascal_case_anim_name); it != timer_conditions.end()) {
            params.frame_factor(it->second.first);
            params.frame_offset(it->second.second);
        }
        else {
            // TODO: do we actually want defaults here? we should probably throw an error earlier in the process
            // Use defaults
            params.frame_factor(anim::default_frame_factor);
            params.frame_offset(anim::default_frame_offset);
        }

        // Convert PascalCase anim name to snake_case for the result key
        const std::string snake_case_anim_name = to_snake_case(pascal_case_anim_name);
        result[snake_case_anim_name] = std::move(params);
    }

    return result;
}

} // namespace

namespace porytiles2 {

ChainableResult<std::map<std::string, AnimationParams>> AnimCodeParser::parse_generated_header(
    const std::filesystem::path &header_path,
    const std::string &tileset_name,
    const std::set<std::string> &expected_anim_names) const
{
    // Type alias hides the comma from the preprocessor (it sees "std::map<std::string, AnimationParams>" as 2 args)
    using ResultType = std::map<std::string, AnimationParams>;
    PT_TRY_ASSIGN_PASS_ERR(
        result,
        parse_animation_params_from_c_file(header_path, tileset_name, expected_anim_names, true, *format_, *diag_),
        ResultType);
    return result;
}

ChainableResult<std::map<std::string, AnimationParams>> AnimCodeParser::parse_vanilla_anims(
    const std::filesystem::path &anims_c_path,
    const std::string &tileset_name,
    const std::set<std::string> &expected_anim_names) const
{
    // Type alias hides the comma from the preprocessor (it sees "std::map<std::string, AnimationParams>" as 2 args)
    using ResultType = std::map<std::string, AnimationParams>;
    PT_TRY_ASSIGN_PASS_ERR(
        result,
        parse_animation_params_from_c_file(anims_c_path, tileset_name, expected_anim_names, false, *format_, *diag_),
        ResultType);
    return result;
}

} // namespace porytiles2
