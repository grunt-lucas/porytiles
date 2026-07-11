#pragma once

#include <string>
#include <vector>

#include "porytiles/utilities/c_parser/source_position.hpp"

namespace porytiles {

/// @brief Represents a parsed INCBIN array declaration from C source code.
///
/// @details
/// IncbinDeclaration captures array declarations that use INCBIN macros to include binary data:
/// @code
/// // Single path:
/// const u32 gTilesetTiles_General[] = INCBIN_U32("data/tilesets/primary/general/tiles.4bpp");
///
/// // Multiple paths (palette arrays):
/// const u16 gTilesetPalettes_General[][16] = {
///     INCBIN_U16("data/tilesets/primary/general/palettes/00.gbapal"),
///     INCBIN_U16("data/tilesets/primary/general/palettes/01.gbapal"),
///     ...
/// };
/// @endcode
///
/// @invariant variable_name_ is never empty
/// @invariant macro_name_ is never empty
/// @invariant paths_ contains at least one path
/// @invariant position_ contains valid 1-based line and column numbers
class IncbinDeclaration {
  public:
    /// @brief Constructs an IncbinDeclaration with a single path.
    ///
    /// @param variable_name The variable name (e.g., "gTilesetTiles_General")
    /// @param macro_name The INCBIN macro name (e.g., "INCBIN_U32")
    /// @param path The path inside the INCBIN macro
    /// @param position The source position of the variable name
    IncbinDeclaration(std::string variable_name, std::string macro_name, std::string path, SourcePosition position)
        : variable_name_{std::move(variable_name)}, macro_name_{std::move(macro_name)}, paths_{std::move(path)},
          position_{position}
    {
    }

    /// @brief Constructs an IncbinDeclaration with multiple paths.
    ///
    /// @details
    /// Used for multi-dimensional arrays like palette declarations where multiple
    /// INCBIN calls are used in a single initializer.
    ///
    /// @param variable_name The variable name (e.g., "gTilesetPalettes_General")
    /// @param macro_name The INCBIN macro name (e.g., "INCBIN_U16")
    /// @param paths The paths from each INCBIN macro call
    /// @param position The source position of the variable name
    IncbinDeclaration(
        std::string variable_name, std::string macro_name, std::vector<std::string> paths, SourcePosition position)
        : variable_name_{std::move(variable_name)}, macro_name_{std::move(macro_name)}, paths_{std::move(paths)},
          position_{position}
    {
    }

    /// @brief Returns the variable name.
    ///
    /// @return A const reference to the variable name (e.g., "gTilesetTiles_General")
    [[nodiscard]] const std::string &variable_name() const
    {
        return variable_name_;
    }

    /// @brief Returns the INCBIN macro name.
    ///
    /// @return A const reference to the macro name (e.g., "INCBIN_U32", "INCBIN_U16")
    [[nodiscard]] const std::string &macro_name() const
    {
        return macro_name_;
    }

    /// @brief Returns all paths from the INCBIN macro(s).
    ///
    /// @return A const reference to the vector of paths
    [[nodiscard]] const std::vector<std::string> &paths() const
    {
        return paths_;
    }

    /// @brief Returns the source position of the variable name.
    ///
    /// @return A const reference to the source position
    [[nodiscard]] const SourcePosition &position() const
    {
        return position_;
    }

    /// @brief Checks if this declaration has multiple paths.
    ///
    /// @return True if there are multiple INCBIN paths (palette-style declaration)
    [[nodiscard]] bool is_multi_path() const
    {
        return paths_.size() > 1;
    }

  private:
    std::string variable_name_;
    std::string macro_name_;
    std::vector<std::string> paths_;
    SourcePosition position_;
};

} // namespace porytiles
