#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>

namespace porytiles {

/// @brief Represents parsed metadata for a single layout from layouts.json.
///
/// @details
/// ProjectLayoutMetadata captures the key fields from a pokeemerald layout definition:
/// @code
/// {
///     "id": "LAYOUT_PETALBURG_CITY",
///     "name": "PetalburgCity_Layout",
///     "width": 20,
///     "height": 20,
///     "primary_tileset": "gTileset_General",
///     "secondary_tileset": "gTileset_Petalburg",
///     "border_filepath": "data/layouts/PetalburgCity/border.bin",
///     "blockdata_filepath": "data/layouts/PetalburgCity/map.bin"
/// }
/// @endcode
///
/// @invariant id_ is never empty
/// @invariant name_ is never empty
/// @invariant primary_tileset_ is never empty
/// @invariant secondary_tileset_ is never empty
class ProjectLayoutMetadata {
  public:
    /// @brief Constructs a ProjectLayoutMetadata from parsed layout fields.
    ///
    /// @param id The layout ID (e.g., "LAYOUT_PETALBURG_CITY")
    /// @param name The layout name (e.g., "PetalburgCity_Layout")
    /// @param width The layout width in metatiles
    /// @param height The layout height in metatiles
    /// @param primary_tileset The primary tileset name (e.g., "gTileset_General")
    /// @param secondary_tileset The secondary tileset name (e.g., "gTileset_Petalburg")
    /// @param border_filepath Relative path to the border file
    /// @param blockdata_filepath Relative path to the blockdata file
    ProjectLayoutMetadata(
        std::string id,
        std::string name,
        std::size_t width,
        std::size_t height,
        std::string primary_tileset,
        std::string secondary_tileset,
        std::filesystem::path border_filepath,
        std::filesystem::path blockdata_filepath)
        : id_{std::move(id)}, name_{std::move(name)}, width_{width}, height_{height},
          primary_tileset_{std::move(primary_tileset)}, secondary_tileset_{std::move(secondary_tileset)},
          border_filepath_{std::move(border_filepath)}, blockdata_filepath_{std::move(blockdata_filepath)}
    {
    }

    [[nodiscard]] const std::string &id() const
    {
        return id_;
    }

    [[nodiscard]] const std::string &name() const
    {
        return name_;
    }

    [[nodiscard]] std::size_t width() const
    {
        return width_;
    }

    [[nodiscard]] std::size_t height() const
    {
        return height_;
    }

    [[nodiscard]] const std::string &primary_tileset() const
    {
        return primary_tileset_;
    }

    [[nodiscard]] const std::string &secondary_tileset() const
    {
        return secondary_tileset_;
    }

    [[nodiscard]] const std::filesystem::path &border_filepath() const
    {
        return border_filepath_;
    }

    [[nodiscard]] const std::filesystem::path &blockdata_filepath() const
    {
        return blockdata_filepath_;
    }

  private:
    std::string id_;
    std::string name_;
    std::size_t width_;
    std::size_t height_;
    std::string primary_tileset_;
    std::string secondary_tileset_;
    std::filesystem::path border_filepath_;
    std::filesystem::path blockdata_filepath_;
};

} // namespace porytiles
