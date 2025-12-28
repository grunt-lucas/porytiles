#pragma once

#include <filesystem>
#include <string>

namespace porytiles2 {

/*
 * TODO: ANIM: this seems like a better fit for infra layer, everything here is related to the pokeemerald project
 * layout. There are no pure domain concepts here.
 */

/**
 * @brief Information extracted from an animation callback function name.
 *
 * @details
 * When a tileset has animations, its callback field in the Tileset struct points to an initialization function (e.g.,
 * `InitTilesetAnim_General` or `InitTilesetAnim_PorytilesManaged_General`). This struct holds the parsed components
 * needed to invoke the AnimCodeParser.
 *
 * The callback function name encodes:
 * - Whether the tileset is Porytiles-managed (uses "PorytilesManaged_" prefix)
 * - The tileset shorthand name (e.g., "General")
 * - The path to the C file containing animation code
 */
class AnimationCallbackInfo {
  public:
    /**
     * @brief Constructs an AnimationCallbackInfo.
     *
     * @param callback_func_name The full callback function name (e.g., "InitTilesetAnim_General")
     * @param tileset_shorthand The tileset name without prefix (e.g., "General")
     * @param porytiles_managed True if this is a Porytiles-managed tileset
     * @param c_file_path Path to the C file containing animation code
     */
    AnimationCallbackInfo(
        std::string callback_func_name,
        std::string tileset_shorthand,
        bool porytiles_managed,
        std::filesystem::path c_file_path)
        : callback_func_name_{std::move(callback_func_name)}, tileset_shorthand_{std::move(tileset_shorthand)},
          porytiles_managed_{porytiles_managed}, c_file_path_{std::move(c_file_path)}
    {
    }

    /**
     * @brief Returns the callback function name.
     *
     * @return A const reference to the callback function name
     */
    [[nodiscard]] const std::string &callback_func_name() const
    {
        return callback_func_name_;
    }

    /**
     * @brief Returns the tileset shorthand name.
     *
     * @return A const reference to the tileset shorthand
     */
    [[nodiscard]] const std::string &tileset_shorthand() const
    {
        return tileset_shorthand_;
    }

    /**
     * @brief Returns whether this is a Porytiles-managed tileset.
     *
     * @return true if Porytiles-managed, false if vanilla
     */
    [[nodiscard]] bool porytiles_managed() const
    {
        return porytiles_managed_;
    }

    /**
     * @brief Returns the path to the C file containing animation code.
     *
     * @return A const reference to the C file path
     */
    [[nodiscard]] const std::filesystem::path &c_file_path() const
    {
        return c_file_path_;
    }

  private:
    std::string callback_func_name_;
    std::string tileset_shorthand_;
    bool porytiles_managed_;
    std::filesystem::path c_file_path_;
};

} // namespace porytiles2
