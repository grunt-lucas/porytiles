#include "porytiles2/infra/services/project_porytiles_tileset_manager.hpp"

#include <filesystem>
#include <fstream>

#include "nlohmann/json.hpp"

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
    const auto original_artifacts_file = artifacts_file(project_root_, tileset_name).parent_path();
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
    // TODO: write original_artifacts.json with the original asset variable names

    /*
     * TODO: Append INCBINs for the Porytiles-managed asset variables
     *
     * In order to do this, we'll need to support the tileset.paths.primary/secondary src/bin configs outlined in the
     * plan. We need to know the paths for the INCBIN. The default for metatiles would be e.g.
     *
     * data/tilesets/primary/{snake_case(shorthand(tileset_name))}/porytiles_bin/metatiles.bin
     */

    // Update headers.h to use Porytiles-managed asset variables
    /*
     * TODO: two cases where we shouldn't update callback here, i.e. pass "update_callback = false"
     *
     * 1. If user requested overwrite_callback: false
     * 2. If callback was already NULL
     */
    return metadata_writer_->update_to_porytiles_managed(tileset_name);
}

} // namespace porytiles2
