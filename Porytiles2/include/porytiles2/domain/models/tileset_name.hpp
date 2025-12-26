#pragma once

#include <string>

#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/string_utils.hpp"

namespace porytiles2 {

/**
 * @brief Represents a canonical tileset name from the decomp's headers.h file.
 *
 * @details
 * In pokeemerald (and related decomps), tilesets are declared in `src/data/tilesets/headers.h` with names following the
 * pattern `gTileset_<Name>` (e.g., `gTileset_General`, `gTileset_Petalburg`). This class enforces that naming
 * convention by validating and encapsulating tileset names.
 *
 * The class provides two factory methods for construction:
 * - `from()` validates a full name (must start with `gTileset_`)
 * - `from_shorthand()` constructs from just the name portion (e.g., "General" → "gTileset_General")
 *
 * This type ensures raw strings don't proliferate through the codebase, providing type safety and a single point of
 * validation for tileset name handling.
 *
 * @invariant The internal name always begins with the `gTileset_` prefix.
 *
 * @see ProjectTilesetArtifactKeyProvider for usage in artifact path resolution
 */
class TilesetName {
  public:
    /**
     * @brief The required prefix for all canonical tileset names.
     */
    static constexpr auto prefix = "gTileset_";

    /**
     * @brief Creates a TilesetName from a full canonical name.
     *
     * @details
     * Validates that the provided name begins with the required `gTileset_` prefix. Use this when parsing names from
     * source files like `headers.h`.
     *
     * @param name The full tileset name (e.g., "gTileset_General")
     * @return A valid TilesetName on success, or an error if the name lacks the required prefix
     */
    static ChainableResult<TilesetName> from(const std::string &name);

    /**
     * @brief Creates a TilesetName from a shorthand name by prepending the prefix.
     *
     * @details
     * Constructs a TilesetName by prepending `gTileset_` to the provided shorthand. Use this when accepting user input
     * or working with abbreviated names.
     *
     * @param shorthand The short name without prefix (e.g., "General")
     * @return A valid TilesetName (e.g., "gTileset_General")
     */
    static ChainableResult<TilesetName> from_shorthand(const std::string &shorthand);

    /**
     * @brief Extracts the shorthand name by removing the prefix.
     *
     * @return The name portion without the `gTileset_` prefix (e.g., "General" from "gTileset_General")
     */
    [[nodiscard]] std::string shorthand() const;

    /**
     * @brief Returns the full canonical tileset name.
     *
     * @return The complete name including prefix (e.g., "gTileset_General")
     */
    [[nodiscard]] std::string name() const
    {
        return name_;
    }

    /**
     * @brief Compares two TilesetName objects for ordering.
     *
     * @details
     * Comparison is based on the full canonical name string. This enables use in ordered containers like std::set and
     * std::map.
     *
     * @param other The TilesetName to compare against
     * @return true if this name is lexicographically less than other
     */
    [[nodiscard]] bool operator<(const TilesetName &other) const
    {
        return name_ < other.name_;
    }

    /**
     * @brief Compares two TilesetName objects for equality.
     *
     * @param other The TilesetName to compare against
     * @return true if both names are identical
     */
    [[nodiscard]] bool operator==(const TilesetName &other) const = default;

  private:
    explicit TilesetName(const std::string &name) : name_{name} {}

    std::string name_;
};

} // namespace porytiles2
