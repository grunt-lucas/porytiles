#include "porytiles2/infra/repos/project_tileset_metadata_provider.hpp"

#include <algorithm>
#include <format>

#include "porytiles2/domain/models/animation.hpp"
#include "porytiles2/utilities/c_parser/c_parser_facade.hpp"
#include "porytiles2/utilities/string_utils.hpp"

namespace {

using namespace porytiles2;

const std::filesystem::path headers_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "headers.h";
const std::filesystem::path graphics_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "graphics.h";
const std::filesystem::path metatiles_rel_path = std::filesystem::path{"src"} / "data" / "tilesets" / "metatiles.h";

// General tileset has its tiles/palettes defined here instead of graphics.h
// TODO: make this configurable
const std::filesystem::path src_graphics_rel_path = std::filesystem::path{"src"} / "graphics.c";

// Tileset animations are defined here for vanilla pokeemerald projects
// TODO: make this configurable
const std::filesystem::path tileset_anims_c_rel_path = std::filesystem::path{"src"} / "tileset_anims.c";

// Prefixes for parsing callback function names
constexpr std::string_view init_tileset_anim_prefix = "InitTilesetAnim_";

/**
 * @brief Extracts tileset shorthand from a callback function name.
 *
 * @details
 * Parses callback function names to extract the tileset identifier:
 * - "InitTilesetAnim_General" -> ("General", false)
 * - "InitTilesetAnim_PorytilesManaged_General" -> ("General", true)
 *
 * @param callback_func The callback function name from tileset metadata
 * @return Pair of (tileset_shorthand, is_porytiles_managed), or empty string on parse failure
 */
[[nodiscard]] std::pair<std::string, bool> extract_tileset_from_callback(const std::string &callback_func)
{
    if (!callback_func.starts_with(init_tileset_anim_prefix)) {
        return {"", false};
    }

    std::string remainder = callback_func.substr(init_tileset_anim_prefix.size());

    if (remainder.starts_with(anim::porytiles_managed_prefix)) {
        return {remainder.substr(anim::porytiles_managed_prefix.size()), true};
    }

    return {remainder, false};
}

/**
 * @brief Parses an INCBIN variable name to extract animation name and frame index.
 *
 * @details
 * Variable naming patterns:
 * - Vanilla: "gTilesetAnims_General_Flower_Frame0" -> ("Flower", 0)
 * - Porytiles: "gTilesetAnims_PorytilesManaged_General_Water_Frame7" -> ("Water", 7)
 *
 * @param var_name The INCBIN variable name
 * @param tileset_shorthand The tileset shorthand (e.g., "General")
 * @param porytiles_managed Whether this is a Porytiles-managed tileset
 * @return Pair of (animation_name, frame_index), or nullopt if not a frame variable
 */
[[nodiscard]] std::optional<std::pair<std::string, std::size_t>>
parse_anim_frame_var(const std::string &var_name, const std::string &tileset_shorthand, bool porytiles_managed)
{
    // Build expected prefix: "gTilesetAnims_[PorytilesManaged_]<Tileset>_"
    std::string prefix = "gTilesetAnims_";
    if (porytiles_managed) {
        prefix += anim::porytiles_managed_prefix;
    }
    prefix += tileset_shorthand + "_";

    if (!var_name.starts_with(prefix)) {
        return std::nullopt;
    }

    std::string remainder = var_name.substr(prefix.size());

    // Find "_Frame" suffix to separate animation name from frame index
    auto frame_pos = remainder.find("_Frame");
    if (frame_pos == std::string::npos) {
        return std::nullopt; // Not a frame variable (might be a pointer array)
    }

    std::string anim_name = remainder.substr(0, frame_pos);
    std::string frame_str = remainder.substr(frame_pos + 6); // Skip "_Frame"

    try {
        std::size_t frame_index = std::stoull(frame_str);
        return std::make_pair(anim_name, frame_index);
    }
    catch (...) {
        return std::nullopt;
    }
}

} // namespace

namespace porytiles2 {

ProjectTilesetMetadataProvider::ProjectTilesetMetadataProvider(
    std::filesystem::path project_root,
    gsl::not_null<const TextFormatter *> format,
    gsl::not_null<const UserDiagnostics *> diag)
    : project_root_{std::move(project_root)}, format_{format}, diag_{diag}
{
}

ChainableResult<void> ProjectTilesetMetadataProvider::ensure_headers_parsed() const
{
    if (headers_parsed_) {
        return {};
    }

    const auto headers_path = project_root_ / headers_rel_path;
    CParserFacade parser{headers_path, format_};

    auto parse_result = parser.parse_struct_initializers(TilesetName::prefix);
    if (!parse_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{format_->format(
                "{}: failed to parse tileset headers", FormatParam{headers_path.string(), Style::bold})},
            parse_result};
    }

    for (auto &struct_decl : parse_result.value()) {
        tileset_structs_.emplace(struct_decl.variable_name(), std::move(struct_decl));
    }

    headers_parsed_ = true;
    return {};
}

ChainableResult<void> ProjectTilesetMetadataProvider::ensure_incbins_parsed() const
{
    if (incbins_parsed_) {
        return {};
    }

    // Parse graphics.h for tiles and palettes
    const auto graphics_path = project_root_ / graphics_rel_path;
    CParserFacade graphics_parser{graphics_path, format_};

    auto graphics_result = graphics_parser.parse_incbin_arrays();
    if (!graphics_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{format_->format(
                "{}: failed to parse graphics INCBINs", FormatParam{graphics_path.string(), Style::bold})},
            graphics_result};
    }

    for (auto &incbin : graphics_result.value()) {
        incbin_vars_.emplace(incbin.variable_name(), std::move(incbin));
    }

    // Parse metatiles.h for metatiles and attributes
    const auto metatiles_path = project_root_ / metatiles_rel_path;
    CParserFacade metatiles_parser{metatiles_path, format_};

    auto metatiles_result = metatiles_parser.parse_incbin_arrays();
    if (!metatiles_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{format_->format(
                "{}: failed to parse metatiles INCBINs", FormatParam{metatiles_path.string(), Style::bold})},
            metatiles_result};
    }

    for (auto &incbin : metatiles_result.value()) {
        incbin_vars_.emplace(incbin.variable_name(), std::move(incbin));
    }

    // Parse src/graphics.c for General tileset (and potentially others with INCBINs there)
    const auto src_graphics_path = project_root_ / src_graphics_rel_path;
    CParserFacade src_graphics_parser{src_graphics_path, format_};

    auto src_graphics_result = src_graphics_parser.parse_incbin_arrays();
    if (!src_graphics_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{format_->format(
                "{}: failed to parse graphics INCBINs", FormatParam{src_graphics_path.string(), Style::bold})},
            src_graphics_result};
    }

    for (auto &incbin : src_graphics_result.value()) {
        incbin_vars_.emplace(incbin.variable_name(), std::move(incbin));
    }

    incbins_parsed_ = true;
    return {};
}

ChainableResult<std::string> ProjectTilesetMetadataProvider::lookup_incbin_path(const std::string &variable_name) const
{
    if (const auto ensure_result = ensure_incbins_parsed(); !ensure_result.has_value()) {
        return ChainableResult<std::string>{
            FormattableError{
                format_->format("failed to look up INCBIN path for '{}'", FormatParam{variable_name, Style::bold})},
            ensure_result};
    }

    const auto it = incbin_vars_.find(variable_name);
    if (it == incbin_vars_.end()) {
        return FormattableError{
            format_->format("INCBIN variable '{}' not found", FormatParam{variable_name, Style::bold})};
    }

    if (it->second.paths().empty()) {
        return FormattableError{
            format_->format("INCBIN variable '{}' has no paths", FormatParam{variable_name, Style::bold})};
    }

    return it->second.paths().front();
}

ChainableResult<std::vector<std::string>>
ProjectTilesetMetadataProvider::lookup_incbin_paths(const std::string &variable_name) const
{
    auto ensure_result = ensure_incbins_parsed();
    if (!ensure_result.has_value()) {
        return ChainableResult<std::vector<std::string>>{
            FormattableError{
                format_->format("failed to look up INCBIN paths for '{}'", FormatParam{variable_name, Style::bold})},
            ensure_result};
    }

    auto it = incbin_vars_.find(variable_name);
    if (it == incbin_vars_.end()) {
        return FormattableError{
            format_->format("INCBIN variable '{}' not found", FormatParam{variable_name, Style::bold})};
    }

    return it->second.paths();
}

ChainableResult<TilesetMetadata> ProjectTilesetMetadataProvider::metadata_for(const TilesetName &tileset_name) const
{
    if (const auto ensure_result = ensure_headers_parsed(); !ensure_result.has_value()) {
        return ChainableResult<TilesetMetadata>{
            FormattableError{format_->format(
                "failed to get metadata for tileset '{}'", FormatParam{tileset_name.name(), Style::bold})},
            ensure_result};
    }

    auto it = tileset_structs_.find(tileset_name.name());
    if (it == tileset_structs_.end()) {
        return FormattableError{
            format_->format("tileset '{}' not found in headers.h", FormatParam{tileset_name.name(), Style::bold})};
    }

    const auto &struct_decl = it->second;

    // Extract isSecondary field
    auto is_secondary_opt = struct_decl.field_value("isSecondary");
    bool is_secondary = is_secondary_opt.has_value() && is_secondary_opt.value() == "TRUE";

    // Extract variable references
    auto tiles_var = struct_decl.field_value("tiles");
    auto palettes_var = struct_decl.field_value("palettes");
    auto metatiles_var = struct_decl.field_value("metatiles");
    auto metatile_attributes_var = struct_decl.field_value("metatileAttributes");
    auto callback_var = struct_decl.field_value("callback");

    if (!tiles_var.has_value()) {
        return FormattableError{
            format_->format("tileset '{}' missing 'tiles' field", FormatParam{tileset_name.name(), Style::bold})};
    }
    if (!palettes_var.has_value()) {
        return FormattableError{
            format_->format("tileset '{}' missing 'palettes' field", FormatParam{tileset_name.name(), Style::bold})};
    }
    if (!metatiles_var.has_value()) {
        return FormattableError{
            format_->format("tileset '{}' missing 'metatiles' field", FormatParam{tileset_name.name(), Style::bold})};
    }
    if (!metatile_attributes_var.has_value()) {
        return FormattableError{format_->format(
            "tileset '{}' missing 'metatileAttributes' field", FormatParam{tileset_name.name(), Style::bold})};
    }

    std::optional<std::string> callback_func = std::nullopt;
    if (callback_var.has_value() && callback_var.value() != "NULL") {
        callback_func = callback_var.value();
    }

    return TilesetMetadata{
        tileset_name,
        is_secondary,
        tiles_var.value(),
        palettes_var.value(),
        metatiles_var.value(),
        metatile_attributes_var.value(),
        callback_func};
}

ChainableResult<TilesetArtifactPaths>
ProjectTilesetMetadataProvider::artifact_paths_for(const TilesetName &tileset_name) const
{
    // First get metadata to get variable names
    auto metadata_result = metadata_for(tileset_name);
    if (!metadata_result.has_value()) {
        return ChainableResult<TilesetArtifactPaths>{
            FormattableError{format_->format(
                "failed to get artifact paths for tileset '{}'", FormatParam{tileset_name.name(), Style::bold})},
            metadata_result};
    }

    const auto &metadata = metadata_result.value();

    // Look up tiles path
    auto tiles_path_result = lookup_incbin_path(metadata.tiles_var());
    if (!tiles_path_result.has_value()) {
        return ChainableResult<TilesetArtifactPaths>{
            FormattableError{format_->format(
                "failed to resolve tiles path for tileset '{}'", FormatParam{tileset_name.name(), Style::bold})},
            tiles_path_result};
    }

    // Look up palette paths
    auto palette_paths_result = lookup_incbin_paths(metadata.palettes_var());
    if (!palette_paths_result.has_value()) {
        return ChainableResult<TilesetArtifactPaths>{
            FormattableError{format_->format(
                "failed to resolve palette paths for tileset '{}'", FormatParam{tileset_name.name(), Style::bold})},
            palette_paths_result};
    }

    // Look up metatiles path
    auto metatiles_path_result = lookup_incbin_path(metadata.metatiles_var());
    if (!metatiles_path_result.has_value()) {
        return ChainableResult<TilesetArtifactPaths>{
            FormattableError{format_->format(
                "failed to resolve metatiles path for tileset '{}'", FormatParam{tileset_name.name(), Style::bold})},
            metatiles_path_result};
    }

    // Look up metatile attributes path
    auto metatile_attributes_path_result = lookup_incbin_path(metadata.metatile_attributes_var());
    if (!metatile_attributes_path_result.has_value()) {
        return ChainableResult<TilesetArtifactPaths>{
            FormattableError{format_->format(
                "failed to resolve metatile attributes path for tileset '{}'",
                FormatParam{tileset_name.name(), Style::bold})},
            metatile_attributes_path_result};
    }

    // Convert string paths to filesystem paths
    std::vector<std::filesystem::path> palette_paths;
    palette_paths.reserve(palette_paths_result.value().size());
    for (const auto &path_str : palette_paths_result.value()) {
        palette_paths.emplace_back(path_str);
    }

    return TilesetArtifactPaths{
        std::filesystem::path{tiles_path_result.value()},
        std::move(palette_paths),
        std::filesystem::path{metatiles_path_result.value()},
        std::filesystem::path{metatile_attributes_path_result.value()}};
}

ChainableResult<bool> ProjectTilesetMetadataProvider::tileset_exists(const TilesetName &tileset_name) const
{
    auto ensure_result = ensure_headers_parsed();
    if (!ensure_result.has_value()) {
        return ChainableResult<bool>{
            FormattableError{format_->format(
                "failed to check if tileset '{}' exists", FormatParam{tileset_name.name(), Style::bold})},
            ensure_result};
    }

    return tileset_structs_.contains(tileset_name.name());
}

ChainableResult<AnimationFramePaths> ProjectTilesetMetadataProvider::parse_anim_incbins_from_file(
    const std::filesystem::path &c_file, const std::string &tileset_shorthand, bool porytiles_managed) const
{
    CParserFacade parser{c_file, format_};

    // Build prefix for filtering: "gTilesetAnims_[PorytilesManaged_]<Tileset>_"
    std::string prefix = "gTilesetAnims_";
    if (porytiles_managed) {
        prefix += anim::porytiles_managed_prefix;
    }
    prefix += tileset_shorthand + "_";

    auto incbins_result = parser.parse_incbin_arrays(prefix);
    if (!incbins_result.has_value()) {
        return ChainableResult<AnimationFramePaths>{
            FormattableError{
                format_->format("{}: failed to parse animation INCBINs", FormatParam{c_file.string(), Style::bold})},
            incbins_result};
    }

    // Group by animation name, collecting (frame_index, path) pairs
    std::map<std::string, std::vector<std::pair<std::size_t, std::filesystem::path>>> grouped;

    for (const auto &incbin : incbins_result.value()) {
        auto parsed = parse_anim_frame_var(incbin.variable_name(), tileset_shorthand, porytiles_managed);
        if (!parsed.has_value()) {
            continue; // Not a frame variable (might be a pointer array)
        }

        auto [anim_name, frame_index] = parsed.value();
        std::string snake_anim_name = to_snake_case(anim_name);

        if (!incbin.paths().empty()) {
            grouped[snake_anim_name].emplace_back(frame_index, std::filesystem::path{incbin.paths().front()});
        }
    }

    // Convert to AnimationFramePaths with ordered vectors
    AnimationFramePaths result;
    for (auto &[anim_name, frames] : grouped) {
        // Sort by frame index
        std::ranges::sort(frames, [](const auto &a, const auto &b) { return a.first < b.first; });

        // Extract paths in order
        std::vector<std::filesystem::path> ordered_paths;
        ordered_paths.reserve(frames.size());
        for (const auto &[idx, path] : frames) {
            ordered_paths.push_back(path);
        }

        result[anim_name] = std::move(ordered_paths);
    }

    return result;
}

ChainableResult<AnimationFramePaths>
ProjectTilesetMetadataProvider::animation_frame_paths_for(const TilesetName &tileset_name) const
{
    // Get metadata to check for animations
    auto metadata_result = metadata_for(tileset_name);
    if (!metadata_result.has_value()) {
        return ChainableResult<AnimationFramePaths>{
            FormattableError{format_->format(
                "failed to get animation paths for tileset '{}'", FormatParam{tileset_name.name(), Style::bold})},
            metadata_result};
    }

    const auto &metadata = metadata_result.value();

    // If no animations, return empty map
    if (!metadata.has_animations()) {
        return AnimationFramePaths{};
    }

    // Extract tileset shorthand from callback function
    auto [tileset_shorthand, porytiles_managed] = extract_tileset_from_callback(metadata.callback_func().value());

    if (tileset_shorthand.empty()) {
        diag_->warning(
            "animation-discovery",
            format_->format(
                "could not parse tileset name from callback '{}'",
                FormatParam{metadata.callback_func().value(), Style::bold}));
        return AnimationFramePaths{};
    }

    // Determine which file to parse
    // Priority: generated_anim_code.h > tileset_anims.c
    auto artifact_paths_result = artifact_paths_for(tileset_name);
    if (!artifact_paths_result.has_value()) {
        return ChainableResult<AnimationFramePaths>{
            FormattableError{"failed to determine tileset root for animation discovery"}, artifact_paths_result};
    }

    const auto tileset_root = project_root_ / artifact_paths_result.value().tileset_root();
    const auto generated_header = tileset_root / "include" / "generated_anim_code.h";

    if (std::filesystem::exists(generated_header)) {
        // Parse Porytiles-managed animations from generated header
        return parse_anim_incbins_from_file(generated_header, tileset_shorthand, true);
    }

    // Fall back to vanilla tileset_anims.c
    const auto anims_c = project_root_ / tileset_anims_c_rel_path;
    if (!std::filesystem::exists(anims_c)) {
        diag_->warning(
            "animation-discovery",
            format_->format("tileset_anims.c not found at '{}'", FormatParam{anims_c.string(), Style::bold}));
        return AnimationFramePaths{};
    }

    return parse_anim_incbins_from_file(anims_c, tileset_shorthand, porytiles_managed);
}

ChainableResult<std::optional<AnimationCallbackInfo>>
ProjectTilesetMetadataProvider::animation_callback_info_for(const TilesetName &tileset_name) const
{
    // Get metadata to check for animations
    auto metadata_result = metadata_for(tileset_name);
    if (!metadata_result.has_value()) {
        return ChainableResult<std::optional<AnimationCallbackInfo>>{
            FormattableError{format_->format(
                "failed to get animation callback info for tileset '{}'",
                FormatParam{tileset_name.name(), Style::bold})},
            metadata_result};
    }

    const auto &metadata = metadata_result.value();

    // If no animations, return nullopt
    if (!metadata.has_animations()) {
        return std::optional<AnimationCallbackInfo>{std::nullopt};
    }

    const std::string &callback_func_name = metadata.callback_func().value();

    // Extract tileset shorthand and porytiles_managed flag from callback function
    /*
     * TODO: this is still not quite right, we shouldn't need to get the tileset_shorthand here
     */
    auto [tileset_shorthand, porytiles_managed] = extract_tileset_from_callback(callback_func_name);

    if (tileset_shorthand.empty()) {
        diag_->warning(
            "animation-discovery",
            format_->format(
                "could not parse tileset name from callback '{}'", FormatParam{callback_func_name, Style::bold}));
        return std::optional<AnimationCallbackInfo>{std::nullopt};
    }

    // Determine which C file contains the animation code
    // Priority: generated_anim_code.h > tileset_anims.c
    auto artifact_paths_result = artifact_paths_for(tileset_name);
    if (!artifact_paths_result.has_value()) {
        return ChainableResult<std::optional<AnimationCallbackInfo>>{
            FormattableError{"failed to determine tileset root for animation callback discovery"},
            artifact_paths_result};
    }

    const auto tileset_root = project_root_ / artifact_paths_result.value().tileset_root();
    const auto generated_header = tileset_root / "include" / "generated_anim_code.h";

    std::filesystem::path c_file_path;
    if (std::filesystem::exists(generated_header)) {
        // Use Porytiles-managed generated header
        c_file_path = generated_header;
        porytiles_managed = true; // Override - generated header is always Porytiles-managed
    }
    else {
        // Fall back to vanilla tileset_anims.c
        c_file_path = project_root_ / tileset_anims_c_rel_path;
        if (!std::filesystem::exists(c_file_path)) {
            diag_->warning(
                "animation-discovery",
                format_->format("tileset_anims.c not found at '{}'", FormatParam{c_file_path.string(), Style::bold}));
            return std::optional<AnimationCallbackInfo>{std::nullopt};
        }
    }

    return std::optional<AnimationCallbackInfo>{
        AnimationCallbackInfo{callback_func_name, tileset_shorthand, porytiles_managed, c_file_path}};
}

} // namespace porytiles2
