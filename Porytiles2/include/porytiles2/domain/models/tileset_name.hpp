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
 * The class provides the `from()` factory method for construction, which accepts either a full canonical name
 * (e.g., "gTileset_General") or just the shorthand portion (e.g., "General"). In both cases, the result is a valid
 * TilesetName with the canonical prefix.
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
     * @brief Creates a TilesetName from either a full canonical name or a shorthand name.
     *
     * @details
     * This factory method accepts flexible input: if the provided name already includes the `gTileset_` prefix, it is
     * used as-is. Otherwise, the prefix is automatically prepended. This allows callers to pass either "General" or
     * "gTileset_General" and receive a valid TilesetName in both cases.
     *
     * @param name Either a full canonical name (e.g., "gTileset_General") or shorthand (e.g., "General")
     * @return A TilesetName with the canonical `gTileset_` prefix
     * @post The returned TilesetName's `name()` method will return a string starting with `gTileset_`
     */
    static TilesetName from(const std::string &name);

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
