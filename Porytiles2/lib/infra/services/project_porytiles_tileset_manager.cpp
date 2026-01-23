#include "porytiles2/infra/services/project_porytiles_tileset_manager.hpp"

#include <filesystem>
#include <fstream>
#include <ranges>

#include "nlohmann/json.hpp"

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/utilities/string_utils.hpp"
#include "porytiles2/xcut/config/config_scope_type.hpp"

namespace {

// TODO: this is hardcoded in multiple places
std::filesystem::path artifacts_file(const std::filesystem::path &project_root, const std::string &tileset_name)
{
    return project_root / "porytiles" / "tilesets" / tileset_name / "tileset-manifest.json";
}

} // namespace

namespace porytiles2 {

ChainableResult<TilesetManifest> ProjectPorytilesTilesetManager::read(const std::string &tileset_name) const
{
    const auto input_path = artifacts_file(project_root_, tileset_name);

    if (!std::filesystem::exists(input_path)) {
        return FormattableError{"{}: file not found", FormatParam{input_path.string(), Style::bold}};
    }

    std::ifstream file{input_path};
    nlohmann::json json_data;
    file >> json_data;

    const auto version = json_data["version"].get<std::uint32_t>();
    const auto imported = json_data["imported"].get<bool>();

    if (imported) {
        return TilesetManifest{
            version,
            json_data[".tiles"].get<std::string>(),
            json_data[".palettes"].get<std::string>(),
            json_data[".metatiles"].get<std::string>(),
            json_data[".metatileAttributes"].get<std::string>(),
            json_data[".callback"].get<std::string>()};
    }

    return TilesetManifest::for_created_tileset(version);
}

void ProjectPorytilesTilesetManager::write(const std::string &tileset_name, const TilesetManifest &artifacts) const
{
    const auto manifest_file = artifacts_file(project_root_, tileset_name);
    std::filesystem::create_directories(manifest_file.parent_path());
    std::ofstream file{manifest_file};

    nlohmann::json json_data;
    json_data["version"] = artifacts.version();
    json_data["imported"] = artifacts.imported();

    if (artifacts.imported()) {
        json_data[".tiles"] = artifacts.tiles();
        json_data[".palettes"] = artifacts.palettes();
        json_data[".metatiles"] = artifacts.metatiles();
        json_data[".metatileAttributes"] = artifacts.metatile_attributes();
        json_data[".callback"] = artifacts.callback();
    }

    file << json_data.dump(2);
    file << std::endl;
}

bool ProjectPorytilesTilesetManager::is_porytiles_managed(const std::string &tileset_name) const
{
    return std::filesystem::exists(artifacts_file(project_root_, tileset_name));
}

namespace {

ChainableResult<void> append_incbin_declarations(
    const IncbinDeclarationAppender *incbin_appender, const std::string &tileset_name, const std::string &bin_path_base)
{
    // Append INCBIN declarations to graphics.h
    auto graphics_result = incbin_appender->append_graphics_declarations(tileset_name, bin_path_base, pal::num_pals);
    if (!graphics_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "Failed to append graphics INCBIN declarations for '{}'.", FormatParam{tileset_name, Style::bold}},
            graphics_result};
    }

    // Append INCBIN declarations to metatiles.h
    auto metatiles_result = incbin_appender->append_metatiles_declarations(tileset_name, bin_path_base);
    if (!metatiles_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "Failed to append metatiles INCBIN declarations for '{}'.", FormatParam{tileset_name, Style::bold}},
            metatiles_result};
    }

    return {};
}

} // namespace

ChainableResult<void> ProjectPorytilesTilesetManager::persist_managed_existing(const std::string &tileset_name) const
{
    // Step 1: Read original metadata from headers.h
    auto metadata_result = metadata_provider_->metadata_for(tileset_name);
    if (!metadata_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"Failed to read metadata for tileset '{}'.", FormatParam{tileset_name, Style::bold}},
            metadata_result};
    }
    const auto &metadata = metadata_result.value();

    // Step 2: Build TilesetManifest from metadata (stores original values for restoration)
    constexpr std::uint32_t version = 1;
    const std::optional<std::string> original_callback_value = metadata.callback_func();

    TilesetManifest artifacts{
        version,
        metadata.tiles_var(),
        metadata.palettes_var(),
        metadata.metatiles_var(),
        metadata.metatile_attributes_var(),
        original_callback_value.value_or("NULL")};

    // Step 3: Write tileset-manifest.json
    write(tileset_name, artifacts);

    // Step 4: Get config values for path computation
    const bool is_secondary = metadata.is_secondary();

    auto bin_path_result = is_secondary
                               ? infra_config_->tileset_paths_secondary_bin(ConfigScopeType::tileset, tileset_name)
                               : infra_config_->tileset_paths_primary_bin(ConfigScopeType::tileset, tileset_name);
    if (!bin_path_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"Failed to get tileset bin path config for '{}'.", FormatParam{tileset_name, Style::bold}},
            bin_path_result};
    }
    const std::string bin_path_base = bin_path_result.value();

    // Step 5: Append INCBIN declarations
    auto incbin_result = append_incbin_declarations(incbin_appender_, tileset_name, bin_path_base);
    if (!incbin_result.has_value()) {
        return incbin_result;
    }

    // Step 6: Determine whether to update callback field
    auto overwrite_callback_result =
        infra_config_->tileset_animations_overwrite_callback(ConfigScopeType::tileset, tileset_name);
    if (!overwrite_callback_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "Failed to get overwrite_callback config for '{}'.", FormatParam{tileset_name, Style::bold}},
            overwrite_callback_result};
    }
    const bool overwrite_callback = overwrite_callback_result.value();

    // Update callback if:
    // 1. User requested overwrite_callback: true, AND
    // 2. Original callback actually had a value (i.e. it wasn't "NULL")
    // Otherwise, leave callback field alone
    const bool should_update_callback = overwrite_callback && original_callback_value.has_value();

    // Step 7: Wire include directive in tileset_anims.c (if updating callback)
    if (should_update_callback) {
        auto wire_result = tileset_anims_modifier_->wire_include_for_tileset(tileset_name, is_secondary);
        if (!wire_result.has_value()) {
            return ChainableResult<void>{
                FormattableError{
                    "Failed to wire tileset_anims.c include for '{}'.", FormatParam{tileset_name, Style::bold}},
                wire_result};
        }
    }

    // Step 8: Update headers.h to use Porytiles-managed asset variables
    return metadata_writer_->update_to_porytiles_managed(tileset_name, should_update_callback);
}

ChainableResult<void> ProjectPorytilesTilesetManager::persist_managed_new(const std::string &tileset_name) const
{
    // Step 1: Create tileset struct in headers.h (it doesn't exist yet)
    auto create_result = metadata_writer_->create_tileset_struct(tileset_name, /*is_secondary=*/false);
    if (!create_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "Failed to create tileset struct in headers.h for '{}'.", FormatParam{tileset_name, Style::bold}},
            create_result};
    }

    // Step 2: Write TilesetManifest with imported=false
    constexpr std::uint32_t version = 1;
    write(tileset_name, TilesetManifest::for_created_tileset(version));

    // Step 3: Get config values for path computation (new tilesets are always primary)
    auto bin_path_result = infra_config_->tileset_paths_primary_bin(ConfigScopeType::tileset, tileset_name);
    if (!bin_path_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"Failed to get tileset bin path config for '{}'.", FormatParam{tileset_name, Style::bold}},
            bin_path_result};
    }
    const std::string bin_path_base = bin_path_result.value();

    // Step 4: Append INCBIN declarations
    auto incbin_result = append_incbin_declarations(incbin_appender_, tileset_name, bin_path_base);
    if (!incbin_result.has_value()) {
        return incbin_result;
    }

    // Step 5: Skip callback wiring for new tilesets (no animations to set up)
    // The headers.h entry was already created with NULL callback

    return {};
}

ChainableResult<void>
ProjectPorytilesTilesetManager::wire_anim_code(const std::string &tileset_name, bool is_secondary) const
{
    /*
     * TODO: update the whole callback handling thing here. Instead of 'tileset.animations.overwrite_callback', let's
     * call the config value 'tileset.animations.wire_anim_code'. If enabled, this wire_anim_code method calls the
     * TilesetAnimsModifier::wire_include_for_tileset and update_callback("callback_name"). If disabled, this method can
     * call TilesetAnimsModifier::remove_include_for_tileset and update_callback("NULL").
     */

    // Determine whether to update callback field
    auto overwrite_callback_result =
        infra_config_->tileset_animations_overwrite_callback(ConfigScopeType::tileset, tileset_name);
    if (!overwrite_callback_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "Failed to get overwrite_callback config for '{}'.", FormatParam{tileset_name, Style::bold}},
            overwrite_callback_result};
    }
    const auto &overwrite_callback = overwrite_callback_result.value();

    if (!overwrite_callback) {
        std::vector<std::string> remark_text;
        remark_text.push_back(format_->format(
            "skipping animation wiring due to configuration:", FormatParam{overwrite_callback, Style::bold}));
        remark_text.emplace_back("");
        std::ranges::copy(overwrite_callback.prettify(*format_), std::back_inserter(remark_text));
        diag_->remark("wire-tileset-animation", remark_text);
        return {};
    }

    // Step 1: Wire include in tileset_anims.c AND declaration in tileset_anims.h
    auto wire_result = tileset_anims_modifier_->wire_include_for_tileset(tileset_name, is_secondary);
    if (!wire_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "Failed to wire tileset_anims include/declaration for '{}'.", FormatParam{tileset_name, Style::bold}},
            wire_result};
    }

    // Step 2: Generate callback function name
    const std::string shorthand = extract_tileset_shorthand(tileset_name);
    const std::string callback_name = "InitTilesetAnim_PorytilesManaged_" + shorthand;

    // Step 3: Update callback field in headers.h
    auto callback_result = metadata_writer_->update_callback(tileset_name, callback_name);
    if (!callback_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "Failed to update callback in headers.h for '{}'.", FormatParam{tileset_name, Style::bold}},
            callback_result};
    }

    return {};
}

} // namespace porytiles2
