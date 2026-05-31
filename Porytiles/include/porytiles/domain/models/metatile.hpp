#pragma once

#include <array>

#include "porytiles/domain/models/layer.hpp"
#include "porytiles/domain/models/pixel_tile.hpp"
#include "porytiles/domain/models/supports_transparency.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/string_utils.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"

namespace porytiles {

namespace metatile {

inline constexpr std::size_t tiles_per_side = 2;
inline constexpr std::size_t tiles_per_metatile_layer = tiles_per_side * tiles_per_side;
inline constexpr std::size_t tiles_per_metatile = tiles_per_metatile_layer * 3;
inline constexpr std::size_t side_length_pix = tiles_per_side * tile::side_length_pix;
inline constexpr std::size_t entries_per_metatile_dual = 8;
inline constexpr std::size_t entries_per_metatile_triple = 12;
inline constexpr std::size_t metatiles_per_row = 8;

enum class Layer : std::uint8_t { bottom = 0, middle = 1, top = 2 };

[[nodiscard]] inline std::string to_string(Layer layer)
{
    switch (layer) {
    case Layer::bottom:
        return "bottom";
    case Layer::middle:
        return "middle";
    case Layer::top:
        return "top";
    }
    panic("unhandled Layer value");
}

inline std::ostream &operator<<(std::ostream &os, const Layer &layer)
{
    os << to_string(layer);
    return os;
}

enum class Subtile : std::uint8_t { northwest = 0, northeast = 1, southwest = 2, southeast = 3 };

[[nodiscard]] inline Subtile subtile_from_index(std::size_t i)
{
    return static_cast<Subtile>(i);
}

[[nodiscard]] inline std::string to_string(Subtile layer)
{
    switch (layer) {
    case Subtile::northwest:
        return "northwest(" + std::to_string(static_cast<std::uint8_t>(layer)) + ")";
    case Subtile::northeast:
        return "northeast(" + std::to_string(static_cast<std::uint8_t>(layer)) + ")";
    case Subtile::southwest:
        return "southwest(" + std::to_string(static_cast<std::uint8_t>(layer)) + ")";
    case Subtile::southeast:
        return "southeast(" + std::to_string(static_cast<std::uint8_t>(layer)) + ")";
    }
    panic("unhandled Subtile value");
}

inline std::ostream &operator<<(std::ostream &os, const Subtile &subtile)
{
    os << to_string(subtile);
    return os;
}

/**
 * @brief Decomposes a global tile index into its metatile index, layer, and subtile position.
 *
 * @details
 * Converts a linear tile index from the global tileset into three components:
 * - The metatile index (which metatile this tile belongs to)
 * - The layer within that metatile (bottom, middle, or top)
 * - The subtile position within that layer (northwest, northeast, southwest, or southeast)
 *
 * Tiles are indexed in metatile-major order: all tiles of metatile 0 come first (bottom layer, then middle layer, then
 * top layer), followed by all tiles of metatile 1, and so on.
 *
 * @param tile_index The global tile index to decompose
 * @return A tuple containing (metatile_index, layer, subtile)
 *
 * @see from_internal_tile_index() for decomposing indices within a single metatile
 */
[[nodiscard]] inline std::tuple<std::size_t, Layer, Subtile> from_tile_index(std::size_t tile_index)
{
    const std::size_t metatile_index = tile_index / tiles_per_metatile;
    const std::size_t local_index = tile_index % tiles_per_metatile;
    const auto layer = static_cast<Layer>(local_index / tiles_per_metatile_layer);
    const auto subtile = static_cast<Subtile>(local_index % tiles_per_metatile_layer);

    return {metatile_index, layer, subtile};
}

/**
 * @brief Decomposes an internal tile index into its layer and subtile position within a metatile.
 *
 * @details
 * Converts a tile index local to a single metatile (range 0-11) into two components:
 * - The layer within the metatile (bottom, middle, or top)
 * - The subtile position within that layer (northwest, northeast, southwest, or southeast)
 *
 * Tiles within a metatile are indexed in layer-major order: bottom layer tiles (0-3), followed by middle layer tiles
 * (4-7), then top layer tiles (8-11).
 *
 * @param tile_index The internal tile index to decompose (must be in range [0, tiles_per_metatile))
 * @pre tile_index < tiles_per_metatile (i.e., tile_index must be in range [0, 11])
 * @return A tuple containing (layer, subtile)
 *
 * @see from_tile_index() for decomposing global tile indices
 */
[[nodiscard]] inline std::tuple<Layer, Subtile> from_internal_tile_index(std::size_t tile_index)
{
    if (tile_index >= tiles_per_metatile) {
        panic("tile_index (" + std::to_string(tile_index) + ") >= tiles_per_metatile");
    }

    const std::size_t local_index = tile_index % tiles_per_metatile;
    const auto layer = static_cast<Layer>(local_index / tiles_per_metatile_layer);
    const auto subtile = static_cast<Subtile>(local_index % tiles_per_metatile_layer);

    return {layer, subtile};
}

[[nodiscard]] inline std::string message_header(
    const TextFormatter &format,
    std::size_t index,
    Layer layer,
    Subtile subtile,
    std::size_t subtile_row,
    std::size_t subtile_col)
{
    return format.format(
        "{} {}({})|{}|{}|{},{}",
        FormatParam{"metatile"},
        FormatParam{int_to_hex_str(index)},
        FormatParam{index},
        FormatParam{to_string(layer)},
        FormatParam{to_string(subtile)},
        FormatParam{std::to_string(subtile_row)},
        FormatParam{std::to_string(subtile_col)});
}

[[nodiscard]] inline std::string
message_header(const TextFormatter &format, std::size_t index, Layer layer, Subtile subtile)
{
    return format.format(
        "{} {}({})|{}|{}",
        FormatParam{"metatile"},
        FormatParam{int_to_hex_str(index)},
        FormatParam{index},
        FormatParam{to_string(layer)},
        FormatParam{to_string(subtile)});
}

[[nodiscard]] inline std::string message_header(const TextFormatter &format, std::size_t index, Subtile subtile)
{
    return format.format(
        "{} {}({})|{}",
        FormatParam{"metatile"},
        FormatParam{int_to_hex_str(index)},
        FormatParam{index},
        FormatParam{to_string(subtile)});
}

} // namespace metatile

/**
 * @brief The core tileset entity - a 2x2 grid of PixelTile objects arranged into three layers.
 *
 * @details
 * Like its component PixelTile objects, the pixel type of Metatile is arbitrary.
 *
 * @invariant Default-constructed Metatile is transparent (satisfies SupportsTransparency design invariant). That is,
 * `Metatile<PixelType>{}` produces a metatile where all PixelTile objects in all three layers are default-constructed
 * (and thus transparent, assuming PixelTile and PixelType satisfy the invariant).
 *
 * @tparam PixelType The pixel type of this Metatile's PixelTile objects
 */
template <SupportsTransparency PixelType>
class Metatile {
  public:
    Metatile() : id_{} {}

    bool operator==(const Metatile &) const = default;

    /**
     * @brief Checks if this entire metatile is transparent (intrinsic transparency only).
     *
     * @details
     * A metatile is transparent if all of its pixels are intrinsically transparent. This overload is only available for
     * pixel types that support parameterless is_transparent() (e.g., IndexPixel).
     *
     * @return True if all tiles in all layers are transparent, false otherwise
     */
    [[nodiscard]] bool is_transparent() const
        requires requires(const PixelType &p) { p.is_transparent(); }
    {
        const bool bottom_transparent =
            std::ranges::all_of(bottom(), [](const auto &tile) { return tile.is_transparent(); });
        const bool middle_transparent =
            std::ranges::all_of(middle(), [](const auto &tile) { return tile.is_transparent(); });
        const bool top_transparent = std::ranges::all_of(top(), [](const auto &tile) { return tile.is_transparent(); });
        return bottom_transparent && middle_transparent && top_transparent;
    }

    /**
     * @brief Checks if this entire metatile is transparent.
     *
     * @details
     * A metatile is transparent if all of its pixels are either intrinsically transparent or are extrinsically
     * transparent, according to the provided extrinsic transparency value. This overload is only available for pixel
     * types that support extrinsic transparency (e.g., Rgba32).
     *
     * @param extrinsic The extrinsic transparency value to check each pixel against
     * @return True if all tiles in all layers are transparent, false otherwise
     */
    [[nodiscard]] bool is_transparent(const PixelType &extrinsic) const
        requires requires(const PixelType &p) { p.is_transparent(p); }
    {
        const bool bottom_transparent =
            std::ranges::all_of(bottom(), [=](const auto &tile) { return tile.is_transparent(extrinsic); });
        const bool middle_transparent =
            std::ranges::all_of(middle(), [=](const auto &tile) { return tile.is_transparent(extrinsic); });
        const bool top_transparent =
            std::ranges::all_of(top(), [=](const auto &tile) { return tile.is_transparent(extrinsic); });
        return bottom_transparent && middle_transparent && top_transparent;
    }

    /**
     * @brief Decomposes this metatile into an array of \link PixelTile PixelTiles \endlink in metatile order.
     *
     * @details
     * Returns tiles in bottom-middle-top layer order, with each layer's tiles arranged sequentially.
     *
     * @return An array containing the constituent tiles
     */
    [[nodiscard]] std::array<PixelTile<PixelType>, metatile::tiles_per_metatile> decompose() const
    {
        std::array<PixelTile<PixelType>, metatile::tiles_per_metatile> tiles{};

        auto out_it = tiles.begin();
        out_it = std::ranges::copy(bottom(), out_it).out;
        out_it = std::ranges::copy(middle(), out_it).out;
        std::ranges::copy(top(), out_it);

        return tiles;
    }

    /**
     * @brief Infers the layer mode (dual or triple) based on metatile content (intrinsic transparency only).
     *
     * @details
     * A metatile uses triple-layer mode if all three layers contain at least one non-transparent pixel.
     * Otherwise, it uses dual-layer mode. This overload uses intrinsic transparency checking.
     *
     * @return LayerMode::triple if all layers have content, LayerMode::dual otherwise
     */
    [[nodiscard]] LayerMode infer_layer_mode() const
        requires requires(const PixelType &p) { p.is_transparent(); }
    {
        return infer_layer_mode_impl([](const PixelType &pixel) { return pixel.is_transparent(); });
    }

    /**
     * @brief Infers the layer mode (dual or triple) based on metatile content.
     *
     * @details
     * A metatile uses triple-layer mode if all three layers contain at least one non-transparent pixel.
     * Otherwise, it uses dual-layer mode. This overload uses both intrinsic and extrinsic transparency checking.
     *
     * @param extrinsic The extrinsic transparency value to check each pixel against
     * @return LayerMode::triple if all layers have content, LayerMode::dual otherwise
     */
    [[nodiscard]] LayerMode infer_layer_mode(const PixelType &extrinsic) const
        requires requires(const PixelType &p) { p.is_transparent(p); }
    {
        return infer_layer_mode_impl([&extrinsic](const PixelType &pixel) { return pixel.is_transparent(extrinsic); });
    }

    /**
     * @brief Infers the layer type based on which layers contain content (intrinsic transparency only).
     *
     * @details
     * Determines which layers (bottom, middle, top) are active for rendering based on content:
     * - Triple-layer mode always returns LayerType::normal
     * - For dual-layer mode, determines the layer type based on which layers have content:
     *   - Bottom only -> covered
     *   - Bottom + middle -> covered
     *   - Bottom + top -> split
     *   - All other combinations -> normal
     *
     * @return The inferred LayerType
     */
    [[nodiscard]] LayerType infer_layer_type() const
        requires requires(const PixelType &p) { p.is_transparent(); }
    {
        return infer_layer_type_impl([](const PixelType &pixel) { return pixel.is_transparent(); });
    }

    /**
     * @brief Infers the layer type based on which layers contain content.
     *
     * @details
     * Determines which layers (bottom, middle, top) are active for rendering based on content:
     * - Triple-layer mode always returns LayerType::normal
     * - For dual-layer mode, determines the layer type based on which layers have content:
     *   - Bottom only -> covered
     *   - Bottom + middle -> covered
     *   - Bottom + top -> split
     *   - All other combinations -> normal
     *
     * @param extrinsic The extrinsic transparency value to check each pixel against
     * @return The inferred LayerType
     */
    [[nodiscard]] LayerType infer_layer_type(const PixelType &extrinsic) const
        requires requires(const PixelType &p) { p.is_transparent(p); }
    {
        return infer_layer_type_impl([&extrinsic](const PixelType &pixel) { return pixel.is_transparent(extrinsic); });
    }

    /**
     * @brief Get a constant reference to a PixelTile from the bottom layer.
     *
     * @details
     * Retrieves the PixelTile at the specified index in the bottom layer array.
     *
     * @param i The index into the bottom layer array (must be 0-3).
     * @return Constant reference to the PixelTile at the specified index.
     */
    [[nodiscard]] const PixelTile<PixelType> &bottom(std::size_t i) const
    {
        if (i > 3) {
            panic(std::format("index {} out of bounds: must be [0,3]", i));
        }
        return bottom_[i];
    }

    /**
     * @brief Get a constant reference to the entire bottom layer array.
     *
     * @details
     * Returns the complete array of tiles in the bottom layer, allowing for range-based iteration.
     *
     * @return Constant reference to the bottom layer tile array.
     */
    [[nodiscard]] const std::array<PixelTile<PixelType>, metatile::tiles_per_metatile_layer> &bottom() const
    {
        return bottom_;
    }

    /**
     * @brief Set a PixelTile in the bottom layer.
     *
     * @details
     * Moves the provided PixelTile into the specified index of the bottom layer array.
     *
     * @param i The index into the bottom layer array (must be 0-3).
     * @param tile The PixelTile to move into the array.
     */
    void set_bottom(std::size_t i, PixelTile<PixelType> tile)
    {
        if (i > 3) {
            panic(std::format("index {} out of bounds: must be [0,3]", i));
        }
        bottom_[i] = std::move(tile);
    }

    /**
     * @brief Get a constant reference to a PixelTile from the middle layer.
     *
     * @details
     * Retrieves the PixelTile at the specified index in the middle layer array.
     *
     * @param i The index into the middle layer array (must be 0-3).
     * @return Constant reference to the PixelTile at the specified index.
     */
    [[nodiscard]] const PixelTile<PixelType> &middle(std::size_t i) const
    {
        if (i > 3) {
            panic(std::format("index {} out of bounds: must be [0,3]", i));
        }
        return middle_[i];
    }

    /**
     * @brief Get a constant reference to the entire middle layer array.
     *
     * @details
     * Returns the complete array of tiles in the middle layer, allowing for range-based iteration.
     *
     * @return Constant reference to the middle layer tile array.
     */
    [[nodiscard]] const std::array<PixelTile<PixelType>, metatile::tiles_per_metatile_layer> &middle() const
    {
        return middle_;
    }

    /**
     * @brief Set a PixelTile in the middle layer.
     *
     * @details
     * Moves the provided PixelTile into the specified index of the middle layer array.
     *
     * @param i The index into the middle layer array (must be 0-3).
     * @param tile The PixelTile to move into the array.
     */
    void set_middle(std::size_t i, PixelTile<PixelType> tile)
    {
        if (i > 3) {
            panic(std::format("index {} out of bounds: must be [0,3]", i));
        }
        middle_[i] = std::move(tile);
    }

    /**
     * @brief Get a constant reference to a PixelTile from the top layer.
     *
     * @details
     * Retrieves the PixelTile at the specified index in the top layer array.
     *
     * @param i The index into the top layer array (must be 0-3).
     * @return Constant reference to the Tile at the specified index.
     */
    [[nodiscard]] const PixelTile<PixelType> &top(std::size_t i) const
    {
        if (i > 3) {
            panic(std::format("index {} out of bounds: must be [0,3]", i));
        }
        return top_[i];
    }

    /**
     * @brief Get a constant reference to the entire top layer array.
     *
     * @details
     * Returns the complete array of tiles in the top layer, allowing for range-based iteration.
     *
     * @return Constant reference to the top layer tile array.
     */
    [[nodiscard]] const std::array<PixelTile<PixelType>, metatile::tiles_per_metatile_layer> &top() const
    {
        return top_;
    }

    /**
     * @brief Set a Tile in the top layer.
     *
     * @details
     * Moves the provided PixelTile into the specified index of the top layer array.
     *
     * @param i The index into the top layer array (must be 0-3).
     * @param tile The PixelTile to move into the array.
     */
    void set_top(std::size_t i, PixelTile<PixelType> tile)
    {
        if (i > 3) {
            panic(std::format("index {} out of bounds: must be [0,3]", i));
        }
        top_[i] = std::move(tile);
    }

  private:
    /**
     * @brief Helper method to check if a layer has any non-transparent content.
     *
     * @details
     * Checks if at least one tile in the given layer array contains at least one non-transparent pixel.
     *
     * @tparam TransparencyPredicate A callable type that takes a PixelType and returns bool
     * @param layer The layer array to check
     * @param is_transparent_pred A predicate function that returns true if a pixel is transparent
     * @return True if the layer has any non-transparent content, false otherwise
     */
    template <typename TransparencyPredicate>
    [[nodiscard]] bool layer_has_content(
        const std::array<PixelTile<PixelType>, metatile::tiles_per_metatile_layer> &layer,
        TransparencyPredicate is_transparent_pred) const
    {
        return std::ranges::any_of(layer, [&is_transparent_pred](const auto &tile) {
            return std::ranges::any_of(
                tile.pix(), [&is_transparent_pred](const auto &pixel) { return !is_transparent_pred(pixel); });
        });
    }

    /**
     * @brief Implementation helper for infer_layer_mode().
     *
     * @details
     * Contains the core layer mode inference logic that works with any transparency predicate.
     *
     * @tparam TransparencyPredicate A callable type that takes a PixelType and returns bool
     * @param is_transparent_pred A predicate function that returns true if a pixel is transparent
     * @return LayerMode::triple if all layers have content, LayerMode::dual otherwise
     */
    template <typename TransparencyPredicate>
    [[nodiscard]] LayerMode infer_layer_mode_impl(TransparencyPredicate is_transparent_pred) const
    {
        const bool bottom_has_content = layer_has_content(bottom_, is_transparent_pred);
        const bool middle_has_content = layer_has_content(middle_, is_transparent_pred);
        const bool top_has_content = layer_has_content(top_, is_transparent_pred);

        if (bottom_has_content && middle_has_content && top_has_content) {
            return LayerMode::triple;
        }
        return LayerMode::dual;
    }

    /**
     * @brief Implementation helper for infer_layer_type().
     *
     * @details
     * Contains the core layer type inference logic that works with any transparency predicate.
     *
     * @tparam TransparencyPredicate A callable type that takes a PixelType and returns bool
     * @param is_transparent_pred A predicate function that returns true if a pixel is transparent
     * @return The inferred LayerType
     */
    template <typename TransparencyPredicate>
    [[nodiscard]] LayerType infer_layer_type_impl(TransparencyPredicate is_transparent_pred) const
    {
        // If triple-layer mode, always return normal
        const LayerMode mode = infer_layer_mode_impl(is_transparent_pred);
        if (mode == LayerMode::triple) {
            return LayerType::normal;
        }

        // For dual-layer mode, determine which layers have content
        const bool bottom_has_content = layer_has_content(bottom_, is_transparent_pred);
        const bool middle_has_content = layer_has_content(middle_, is_transparent_pred);
        const bool top_has_content = layer_has_content(top_, is_transparent_pred);

        // Apply the case logic
        if (bottom_has_content && !middle_has_content && top_has_content) {
            // Case 6: bottom/top content -> split
            return LayerType::split;
        }
        if (bottom_has_content && (middle_has_content || !top_has_content)) {
            // Case 1: bottom only -> covered
            // Case 5: bottom/middle content -> covered
            return LayerType::covered;
        }
        // All other cases (including no content) -> normal
        // Case 0: completely transparent -> normal
        // Case 2: middle only -> normal
        // Case 3: top only -> normal
        // Case 4: middle/top content -> normal
        return LayerType::normal;
    }

    std::array<PixelTile<PixelType>, metatile::tiles_per_metatile_layer> bottom_;
    std::array<PixelTile<PixelType>, metatile::tiles_per_metatile_layer> middle_;
    std::array<PixelTile<PixelType>, metatile::tiles_per_metatile_layer> top_;
    std::size_t id_;
};

namespace metatile {

template <typename T>
[[nodiscard]] std::vector<PixelTile<T>> decompose(const std::vector<Metatile<T>> &metatiles)
{
    std::vector<PixelTile<T>> tiles;
    tiles.reserve(metatiles.size() * tiles_per_metatile);
    for (const auto &mt : metatiles) {
        tiles.append_range(mt.decompose());
    }
    return tiles;

    // This is a less performant version that uses std::ranges
    //
    // auto tiles = metatiles
    //     | std::views::transform([](const auto& mt) { return mt.decompose(); })
    //     | std::views::join
    //     | std::ranges::to<std::vector>();
    //
    // This transforms each Metatile to a vector<Tile>, flattens (joins) all the vectors together, and collects into a
    // final vector<Tile>.
}

} // namespace metatile

} // namespace porytiles
