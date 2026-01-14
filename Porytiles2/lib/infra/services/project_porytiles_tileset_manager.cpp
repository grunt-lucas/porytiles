#include "porytiles2/infra/services/project_porytiles_tileset_manager.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>

#include "nlohmann/json.hpp"

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/xcut/config/config_scope_type.hpp"

namespace {

// TODO: this is hardcoded in multiple places
std::filesystem::path artifacts_file(const std::filesystem::path &project_root, const std::string &tileset_name)
{
    return project_root / "porytiles" / "tilesets" / tileset_name / "original_artifacts.json";
}

} // namespace

namespace porytiles2 {

ChainableResult<OriginalArtifacts> ProjectPorytilesTilesetManager::read(const std::string &tileset_name) const
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
        return OriginalArtifacts{
            version,
            json_data[".tiles"].get<std::string>(),
            json_data[".palettes"].get<std::string>(),
            json_data[".metatiles"].get<std::string>(),
            json_data[".metatileAttributes"].get<std::string>(),
            json_data[".callback"].get<std::string>()};
    }

    return OriginalArtifacts::for_created_tileset(version);
}

void ProjectPorytilesTilesetManager::write(const std::string &tileset_name, const OriginalArtifacts &artifacts) const
{
    const auto original_artifacts_file = artifacts_file(project_root_, tileset_name);
    std::filesystem::create_directories(original_artifacts_file.parent_path());
    std::ofstream file{original_artifacts_file};

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
}

bool ProjectPorytilesTilesetManager::is_porytiles_managed(const std::string &tileset_name) const
{
    return std::filesystem::exists(artifacts_file(project_root_, tileset_name));
}

ChainableResult<void> ProjectPorytilesTilesetManager::persist_managed_state(const std::string &tileset_name) const
{
    // Step 1: Read original metadata from headers.h
    auto metadata_result = metadata_provider_->metadata_for(tileset_name);
    if (!metadata_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"failed to read metadata for tileset '{}'", FormatParam{tileset_name, Style::bold}},
            metadata_result};
    }
    const auto &metadata = metadata_result.value();

    // Step 2: Build OriginalArtifacts from metadata
    constexpr std::uint32_t version = 1;
    const std::optional<std::string> original_callback_value = metadata.callback_func();

    OriginalArtifacts artifacts{
        version,
        metadata.tiles_var(),
        metadata.palettes_var(),
        metadata.metatiles_var(),
        metadata.metatile_attributes_var(),
        original_callback_value.value_or("NULL")};

    // Step 3: Write original_artifacts.json
    write(tileset_name, artifacts);

    // Step 4: Get config values for path computation
    const bool is_secondary = metadata.is_secondary();

    auto bin_path_result = is_secondary
                               ? infra_config_->tileset_paths_secondary_bin(ConfigScopeType::tileset, tileset_name)
                               : infra_config_->tileset_paths_primary_bin(ConfigScopeType::tileset, tileset_name);
    if (!bin_path_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{"failed to get tileset bin path config for '{}'", FormatParam{tileset_name, Style::bold}},
            bin_path_result};
    }
    const std::string bin_path_base = bin_path_result.value();

    // Step 5: Append INCBIN declarations to graphics.h
    auto graphics_result = incbin_appender_->append_graphics_declarations(tileset_name, bin_path_base, pal::num_pals);
    if (!graphics_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "failed to append graphics INCBIN declarations for '{}'", FormatParam{tileset_name, Style::bold}},
            graphics_result};
    }

    // Step 6: Append INCBIN declarations to metatiles.h
    auto metatiles_result = incbin_appender_->append_metatiles_declarations(tileset_name, bin_path_base);
    if (!metatiles_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "failed to append metatiles INCBIN declarations for '{}'", FormatParam{tileset_name, Style::bold}},
            metatiles_result};
    }

    // Step 7: Determine whether to update callback field
    auto overwrite_callback_result =
        infra_config_->tileset_animations_overwrite_callback(ConfigScopeType::tileset, tileset_name);
    if (!overwrite_callback_result.has_value()) {
        return ChainableResult<void>{
            FormattableError{
                "failed to get overwrite_callback config for '{}'", FormatParam{tileset_name, Style::bold}},
            overwrite_callback_result};
    }
    const bool overwrite_callback = overwrite_callback_result.value();

    // Update callback if:
    // 1. User requested overwrite_callback: true, AND
    // 2. Original callback actually had a value (i.e. it wasn't "NULL")
    // Otherwise, leave callback field alone
    const bool should_update_callback = overwrite_callback && original_callback_value.has_value();

    // Step 8: Update headers.h to use Porytiles-managed asset variables
    return metadata_writer_->update_to_porytiles_managed(tileset_name, should_update_callback);
}

} // namespace porytiles2
