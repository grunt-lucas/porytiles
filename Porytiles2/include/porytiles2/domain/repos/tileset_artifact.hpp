#pragma once

#include <optional>
#include <string>

namespace porytiles2 {

/**
 * @brief Represents a Pokémon Generation III decomp tileset artifact with type and optional metadata.
 *
 * @details
 * A tileset artifact is a file or data component that belongs to a tileset. Each artifact has a specific type (such as
 * PNG images, binary data, or CSV files) and may have optional metadata like a name (for animations) or index (for
 * palette overrides or animation frames).
 *
 * Examples:
 * - A metatiles.bin file would be Type::metatiles_bin with no additional metadata
 * - An animation frame might be Type::porytiles_anim_frame with name "water" and index 2
 * - A palette override could be Type::override_n with index 3
 */
class TilesetArtifact {
  public:
    /**
     * @brief Enumeration of all supported tileset artifact types.
     *
     * @details
     * These types represent the various file formats and data components that make up a complete tileset. The artifacts
     * fall into several categories:
     * - Input images: bottom_png, middle_png, top_png (Porytiles source layers)
     * - Configuration: attributes_csv (metatile attribute overrides)
     * - Animations: porytiles_anim_frame, porymap_anim_frame
     * - Palette data: override_n, pal_n
     * - Output binaries: metatiles_bin, metatile_attributes_bin, tiles_png (Porymap-compatible formats)
     */
    enum class Type {
        bottom_png,              ///< Bottom layer PNG input image
        middle_png,              ///< Middle layer PNG input image
        top_png,                 ///< Top layer PNG input image
        attributes_csv,          ///< CSV file containing metatile attribute overrides
        porytiles_anim_frame,    ///< Animation frame PNG for Porytiles-format animation
        pal_override_n,          ///< JASC palette override file
        metatiles_bin,           ///< Metatile data output for Porymap
        metatile_attributes_bin, ///< Metatile attributes output for Porymap
        tiles_png,               ///< Combined tile sheet PNG output for Porymap
        porymap_anim_frame,      ///< Animation frame PNG for Porymap-format animation
        pal_n                    ///< JASC palette data file
        // pal_hint_n,            // TODO: pal hints could be like Porytiles1's palette primers?
    };

    /**
     * @brief Constructs a tileset artifact with only a type.
     *
     * @param type The artifact type
     */
    explicit TilesetArtifact(const Type type) : type_{type}, name_{std::nullopt}, index_{std::nullopt} {}

    /**
     * @brief Constructs a tileset artifact with a type and name.
     *
     * @details
     * Typically used for animation artifacts where the name identifies the animation (e.g., "water", "flowers").
     *
     * @param type The artifact type
     * @param name The name associated with the artifact
     */
    explicit TilesetArtifact(const Type type, std::string name)
        : type_{type}, name_{std::move(name)}, index_{std::nullopt}
    {
    }

    /**
     * @brief Constructs a tileset artifact with a type and index.
     *
     * @details
     * Typically used for indexed artifacts like palette overrides or animation frames.
     *
     * @param type The artifact type
     * @param index The index associated with the artifact
     */
    explicit TilesetArtifact(const Type type, unsigned int index) : type_{type}, name_{std::nullopt}, index_{index} {}

    /**
     * @brief Constructs a tileset artifact with a type, name, and index.
     *
     * @details
     * Used for artifacts that require both identification by name and indexing, such as specific frames within a named
     * animation.
     *
     * @param type The artifact type
     * @param name The name associated with the artifact
     * @param index The index associated with the artifact
     */
    explicit TilesetArtifact(const Type type, std::string name, int index)
        : type_{type}, name_{std::move(name)}, index_{index}
    {
    }

    /**
     * @brief Gets the artifact type.
     *
     * @return The type of this artifact
     */
    [[nodiscard]] Type type() const
    {
        return type_;
    }

    /**
     * @brief Gets the artifact name if present.
     *
     * @return Optional name string, or nullopt if the artifact has no associated name
     */
    [[nodiscard]] std::optional<std::string> name() const
    {
        return name_;
    }

    /**
     * @brief Gets the artifact index if present.
     *
     * @return Optional index value, or nullopt if the artifact has no associated index
     */
    [[nodiscard]] std::optional<unsigned int> index() const
    {
        return index_;
    }

  private:
    Type type_;
    /*
     * TODO: this isn't the best design, compiler can no longer catch programmer errors. If programmer forgets to
     * provide a name/index, Porytiles has to panic which is not great. Ideally we'd like the compiler to help the
     * programmer here.
     */
    std::optional<std::string> name_;
    std::optional<unsigned int> index_;
};

} // namespace porytiles2
