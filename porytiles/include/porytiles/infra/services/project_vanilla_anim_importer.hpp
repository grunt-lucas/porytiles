#pragma once

#include <filesystem>
#include <map>
#include <string>

#include "gsl/pointers"

#include "porytiles/domain/models/animation.hpp"
#include "porytiles/domain/models/index_pixel.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief Imports vanilla animation metadata and frame data as IndexPixel tiles.
///
/// @details
/// ProjectVanillaAnimImporter reads animation configuration and frame PNG files from a vanilla pokeemerald project.
///
/// Workflow:
/// 1. Gets tileset metadata (callback function name) from headers.h
/// 2. Parses tileset_anims.c with AnimCodeParser to get AnimParams
/// 3. Parses INCBIN declarations to find PNG frame file paths
/// 4. Loads frame PNG files and extracts IndexPixel tiles
///
/// This importer does NOT extract key frames from tiles.png - that responsibility belongs to AnimDecompiler, which
/// needs to understand VRAM layout and palette assignment.
///
/// @note Returned animations will have has_key_frame() == false
class ProjectVanillaAnimImporter {
  public:
    /// @brief Constructs a ProjectVanillaAnimImporter.
    ///
    /// @param project_root The path to the pokeemerald project root directory
    /// @param format Formatter for error message styling (non-owning, must outlive importer)
    /// @param diag UserDiagnostics for warnings and info messages (non-owning, must outlive importer)
    ProjectVanillaAnimImporter(
        std::filesystem::path project_root,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag)
        : project_root_{std::move(project_root)}, format_{format}, diag_{diag}
    {
    }

    /// @brief Import animations from a vanilla tileset.
    ///
    /// @details
    /// Parses tileset_anims.c to discover animation names, parameters, and frame paths. Loads frame PNG files and
    /// extracts IndexPixel tiles for each frame. Does not set key_frame.
    ///
    /// @param tileset_name The name of the tileset (e.g., "gTileset_General")
    /// @pre Tileset must exist and have animations defined in tileset_anims.c
    /// @return Map of animation names (snake_case) to Animation<IndexPixel> objects, or error
    /// @post Each returned Animation has has_key_frame() == false
    /// @post Each returned Animation has frames populated with IndexPixel tile data
    [[nodiscard]] ChainableResult<std::map<std::string, Animation<IndexPixel>>>
    import_animations(const std::string &tileset_name) const;

  private:
    std::filesystem::path project_root_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;
};

} // namespace porytiles
