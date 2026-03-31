#include "porytiles2/infra/services/project_porytiles_tileset_manager.hpp"

#include <filesystem>
#include <fstream>
#include <ranges>
#include <string_view>

#include "nlohmann/json.hpp"

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/utilities/string_utils.hpp"
#include "porytiles2/xcut/config/config_scope_type.hpp"

namespace {

using namespace porytiles2;

// TODO: this is hardcoded in multiple places
std::filesystem::path artifacts_file(const std::filesystem::path &project_root, const std::string &tileset_name)
{
    return project_root / "porytiles" / "tilesets" / tileset_name / "tileset-manifest.json";
}

constexpr std::string_view porytiles_managed_callback_prefix = "InitTilesetAnim_PorytilesManaged_";

[[nodiscard]] bool is_porytiles_managed_callback(const std::optional<std::string> &callback)
{
    if (!callback.has_value()) {
        return false;
    }
    return callback->starts_with(porytiles_managed_callback_prefix);
}

ChainableResult<void> append_incbin_declarations(
    const IncbinDeclarationAppender *incbin_appender,
    const std::string &tileset_name,
    const std::string &bin_path_base,
    std::size_t metatile_attr_size)
{
    // Append INCBIN declarations to graphics.h
    PT_TRY_CALL_CHAIN_ERR(
        incbin_appender->append_graphics_declarations(tileset_name, bin_path_base, pal::num_pals),
        void,
        "Failed to append graphics INCBIN declarations for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // Append INCBIN declarations to metatiles.h
    PT_TRY_CALL_CHAIN_ERR(
        incbin_appender->append_metatiles_declarations(tileset_name, bin_path_base, metatile_attr_size),
        void,
        "Failed to append metatiles INCBIN declarations for '{}'.",
        FormatParam(tileset_name, Style::bold));

    return {};
}

} // namespace

namespace porytiles2 {

ChainableResult<TilesetManifest> ProjectPorytilesTilesetManager::read(const std::string &tileset_name) const
{
    const auto input_path = artifacts_file(project_root_, tileset_name);

    if (!std::filesystem::exists(input_path)) {
        return FormattableError{"'{}': File not found.", FormatParam{input_path.string(), Style::bold}};
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

ChainableResult<void> ProjectPorytilesTilesetManager::persist_managed_existing(const std::string &tileset_name) const
{
    // Step 1: Read original metadata from headers.h
    PT_TRY_ASSIGN_CHAIN_ERR(
        metadata,
        metadata_provider_->metadata_for(tileset_name),
        void,
        "Failed to read metadata for tileset '{}'.",
        FormatParam(tileset_name, Style::bold));

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

    PT_TRY_ASSIGN_CHAIN_ERR(
        bin_path_base,
        is_secondary ? infra_config_->tileset_paths_secondary_bin(ConfigScopeType::tileset, tileset_name)
                     : infra_config_->tileset_paths_primary_bin(ConfigScopeType::tileset, tileset_name),
        void,
        "Failed to get tileset bin path config for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // Step 5: Resolve metatile attribute size from config
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatile_attr_size,
        infra_config_->metatile_attr_size(ConfigScopeType::tileset, tileset_name),
        void,
        "Failed to get metatile attribute size config for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // Step 6: Append INCBIN declarations
    auto incbin_result = append_incbin_declarations(incbin_appender_, tileset_name, bin_path_base, metatile_attr_size);
    if (!incbin_result.has_value()) {
        return incbin_result;
    }

    // Step 7: Update headers.h to use Porytiles-managed asset variables
    // Note: Callback update is handled separately by wire_anim_code() in the use-case layer
    return metadata_writer_->update_to_porytiles_managed(tileset_name);
}

ChainableResult<void> ProjectPorytilesTilesetManager::persist_managed_new(const std::string &tileset_name) const
{
    // Step 1: Create tileset struct in headers.h (it doesn't exist yet)
    PT_TRY_CALL_CHAIN_ERR(
        metadata_writer_->create_tileset_struct(tileset_name, /*is_secondary=*/false),
        void,
        "Failed to create tileset struct in headers.h for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // Step 2: Write TilesetManifest with imported=false
    constexpr std::uint32_t version = 1;
    write(tileset_name, TilesetManifest::for_created_tileset(version));

    // Step 3: Get config values for path computation (new tilesets are always primary)
    PT_TRY_ASSIGN_CHAIN_ERR(
        bin_path_base,
        infra_config_->tileset_paths_primary_bin(ConfigScopeType::tileset, tileset_name),
        void,
        "Failed to get tileset bin path config for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // Step 4: Resolve metatile attribute size from config
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatile_attr_size,
        infra_config_->metatile_attr_size(ConfigScopeType::tileset, tileset_name),
        void,
        "Failed to get metatile attribute size config for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // Step 5: Append INCBIN declarations
    auto incbin_result = append_incbin_declarations(incbin_appender_, tileset_name, bin_path_base, metatile_attr_size);
    if (!incbin_result.has_value()) {
        return incbin_result;
    }

    return {};
}

ChainableResult<void>
ProjectPorytilesTilesetManager::wire_anim_code(const std::string &tileset_name, bool is_secondary) const
{
    // Check config - if wire_anim_code is disabled, delegate to remove
    PT_TRY_ASSIGN_CHAIN_ERR(
        should_wire,
        infra_config_->tileset_animations_wire_anim_code(ConfigScopeType::tileset, tileset_name),
        void,
        "Failed to get wire_anim_code config for '{}'.",
        FormatParam(tileset_name, Style::bold));

    if (!should_wire) {
        std::vector<std::string> remark_text;
        remark_text.emplace_back("Config 'tileset.animations.wire_anim_code' is false, removing any existing wiring.");
        remark_text.emplace_back("");
        std::ranges::copy(should_wire.prettify(diag_->formatter()), std::back_inserter(remark_text));
        diag_->remark("wire-tileset-animation", remark_text);
        return remove_wired_anim_code(tileset_name, is_secondary);
    }

    // Step 1: Wire include in tileset_anims.c AND declaration in tileset_anims.h
    PT_TRY_CALL_CHAIN_ERR(
        tileset_anims_modifier_->wire_include_for_tileset(tileset_name, is_secondary),
        void,
        "Failed to wire tileset_anims include/declaration for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // Step 2: Generate callback function name
    const std::string shorthand = extract_tileset_shorthand(tileset_name);
    const std::string callback_name = "InitTilesetAnim_PorytilesManaged_" + shorthand;

    // Step 3: Update callback field in headers.h
    PT_TRY_CALL_CHAIN_ERR(
        metadata_writer_->update_callback(tileset_name, callback_name),
        void,
        "Failed to update callback in headers.h for '{}'.",
        FormatParam(tileset_name, Style::bold));

    return {};
}

ChainableResult<void>
ProjectPorytilesTilesetManager::remove_wired_anim_code(const std::string &tileset_name, bool is_secondary) const
{
    // Step 1: Remove include from tileset_anims.c and declaration from tileset_anims.h
    PT_TRY_CALL_CHAIN_ERR(
        tileset_anims_modifier_->remove_include_for_tileset(tileset_name, is_secondary),
        void,
        "Failed to remove animation includes for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // Step 2: Only update callback to NULL if it's a Porytiles-managed callback
    // This preserves user-managed callbacks when wire_anim_code is false
    PT_TRY_ASSIGN_CHAIN_ERR(
        metadata,
        metadata_provider_->metadata_for(tileset_name),
        void,
        "Failed to read metadata for '{}'.",
        FormatParam(tileset_name, Style::bold));

    const auto &current_callback = metadata.callback_func();
    if (is_porytiles_managed_callback(current_callback)) {
        PT_TRY_CALL_CHAIN_ERR(
            metadata_writer_->update_callback(tileset_name, "NULL"),
            void,
            "Failed to update callback to NULL for '{}'.",
            FormatParam(tileset_name, Style::bold));
    }

    return {};
}

} // namespace porytiles2
