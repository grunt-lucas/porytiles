#pragma once

#include <compare>
#include <cstddef>
#include <string>
#include <variant>

#include "porytiles2/domain/models/color_set.hpp"
#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

/**
 * @brief Wraps a ColorSet with a tile ID for tracking during palette packing.
 *
 * @details
 * PackableTile represents a tile that needs to be assigned to a hardware palette. It associates a unique tile ID with
 * the set of colors present in that tile, enabling the palette packer to track which tiles have been assigned to which
 * palettes.
 *
 * Three types of tiles are supported:
 * - HintId: Tiles created from \link PaletteHint PaletteHints\endlink, identified by a string name
 * - PrefilledPaletteId: Tiles created from \link PrefilledPalette PrefilledPalettes\endlink, identified by palette
 * index
 * - RegularId: Regular input tiles, identified by tile index
 *
 * PackableTiles are sortable with the following order:
 * 1. Hint tiles (lexicographic by name)
 * 2. Prefilled palette tiles (ascending by index)
 * 3. Regular tiles (ascending by index)
 */
class PackableTile {
  public:
    /**
     * @brief Identifies a tile created from a palette hint.
     */
    struct HintId {
        std::string name;
        [[nodiscard]] auto operator<=>(const HintId &) const = default;
        [[nodiscard]] bool operator==(const HintId &) const = default;
    };

    /**
     * @brief Identifies a tile created from a prefilled palette.
     *
     * @invariant index is always in the range [0, 15]
     */
    class PrefilledPaletteId {
      public:
        static constexpr std::size_t max_index = 15;

        explicit PrefilledPaletteId(std::size_t index) : index_{index}
        {
            if (index > max_index) {
                panic(
                    "PrefilledPaletteId index out of range: " + std::to_string(index) + " > " +
                    std::to_string(max_index));
            }
        }

        [[nodiscard]] std::size_t index() const
        {
            return index_;
        }

        [[nodiscard]] auto operator<=>(const PrefilledPaletteId &) const = default;
        [[nodiscard]] bool operator==(const PrefilledPaletteId &) const = default;

      private:
        std::size_t index_;
    };

    /**
     * @brief Identifies a tile created from an animation.
     */
    struct AnimId {
        std::string name;
        std::size_t subtile_index;
        [[nodiscard]] auto operator<=>(const AnimId &) const = default;
        [[nodiscard]] bool operator==(const AnimId &) const = default;
    };

    /**
     * @brief Identifies a regular input tile.
     */
    struct RegularId {
        std::size_t index;
        [[nodiscard]] auto operator<=>(const RegularId &) const = default;
        [[nodiscard]] bool operator==(const RegularId &) const = default;
    };

    /**
     * @brief Variant type for tile identification.
     *
     * @details
     * The order of alternatives determines sorting: HintId < PrefilledPaletteId < RegularId.
     */
    using Id = std::variant<HintId, PrefilledPaletteId, RegularId, AnimId>;

    /**
     * @brief Constructs a PackableTile from a palette hint.
     *
     * @param id The hint identifier
     * @param color_set The set of colors present in this tile
     */
    PackableTile(HintId id, ColorSet color_set);

    /**
     * @brief Constructs a PackableTile from a prefilled palette.
     *
     * @param id The prefilled palette identifier
     * @param color_set The set of colors present in this tile
     */
    PackableTile(PrefilledPaletteId id, ColorSet color_set);

    /**
     * @brief Constructs a PackableTile from a regular input tile.
     *
     * @param id The regular tile identifier
     * @param color_set The set of colors present in this tile
     */
    PackableTile(RegularId id, ColorSet color_set);

    /**
     * @brief Constructs a PackableTile from an animation.
     *
     * @param id The anim identifier
     * @param color_set The set of colors present in this tile
     */
    PackableTile(AnimId id, ColorSet color_set);

    /**
     * @brief Constructs a PackableTile from an Id variant.
     *
     * @details
     * This constructor allows direct construction from a type-erased Id variant, useful when the specific ID type is
     * not known at compile time (e.g., when reconstructing tiles from stored IDs).
     *
     * @param id The tile identifier variant
     * @param color_set The set of colors present in this tile
     */
    PackableTile(Id id, ColorSet color_set);

    /**
     * @brief Gets the tile's identifier variant.
     *
     * @return A const reference to the Id variant
     */
    [[nodiscard]] const Id &id() const
    {
        return id_;
    }

    /**
     * @brief Gets the hint name for hint tiles.
     *
     * @pre is_hint() must be true
     * @return The hint name
     */
    [[nodiscard]] const std::string &hint_name() const;

    /**
     * @brief Gets the prefilled palette index for prefilled palette tiles.
     *
     * @pre is_prefilled_palette() must be true
     * @return The prefilled palette index
     */
    [[nodiscard]] std::size_t prefilled_index() const;

    /**
     * @brief Gets the tile index for regular tiles.
     *
     * @pre is_regular() must be true
     * @return The regular tile index
     */
    [[nodiscard]] std::size_t regular_index() const;

    /**
     * @brief Checks if this is a hint tile.
     *
     * @return true if the tile was created from a palette hint
     */
    [[nodiscard]] bool is_hint() const
    {
        return std::holds_alternative<HintId>(id_);
    }

    /**
     * @brief Checks if this is a prefilled palette tile.
     *
     * @return true if the tile was created from a prefilled palette
     */
    [[nodiscard]] bool is_prefilled_palette() const
    {
        return std::holds_alternative<PrefilledPaletteId>(id_);
    }

    /**
     * @brief Checks if this is a regular tile.
     *
     * @return true if the tile is a regular input tile
     */
    [[nodiscard]] bool is_regular() const
    {
        return std::holds_alternative<RegularId>(id_);
    }

    /**
     * @brief Gets the set of colors present in this tile.
     *
     * @return A const reference to the ColorSet
     */
    [[nodiscard]] const ColorSet &color_set() const
    {
        return color_set_;
    }

    /**
     * @brief Gets the number of colors in this tile.
     *
     * @details
     * This is a convenience method that returns the count of colors in the tile's ColorSet.
     *
     * @return The number of colors in the tile
     */
    [[nodiscard]] std::size_t color_count() const;

    /**
     * @brief Three-way comparison operator.
     *
     * @details
     * Compares tiles by their Id only. The variant ordering ensures hint tiles come first,
     * then prefilled palette tiles, then regular tiles.
     *
     * @param other The tile to compare against
     * @return The ordering relationship
     */
    [[nodiscard]] auto operator<=>(const PackableTile &other) const
    {
        return id_ <=> other.id_;
    }

    /**
     * @brief Equality comparison operator.
     *
     * @param other The tile to compare against
     * @return true if the tiles have the same Id
     */
    [[nodiscard]] bool operator==(const PackableTile &other) const
    {
        return id_ == other.id_;
    }

  private:
    Id id_;
    ColorSet color_set_;
};

inline std::string to_string(const PackableTile::Id &id)
{
    return std::visit(
        []<typename IdVariant>(const IdVariant &value) -> std::string {
            using T = std::decay_t<IdVariant>;
            if constexpr (std::is_same_v<T, PackableTile::HintId>) {
                return "Hint(" + value.name + ")";
            }
            else if constexpr (std::is_same_v<T, PackableTile::PrefilledPaletteId>) {
                return "Prefilled(" + std::to_string(value.index()) + ")";
            }
            else if constexpr (std::is_same_v<T, PackableTile::RegularId>) {
                return "Regular(" + std::to_string(value.index) + ")";
            }
            else if constexpr (std::is_same_v<T, PackableTile::AnimId>) {
                return "Anim(" + value.name + ", " + std::to_string(value.subtile_index) + ")";
            }
            else {
                static_assert(sizeof(T) == 0, "Unhandled PackableTile::Id variant alternative");
                panic("Unhandled PackableTile::Id variant alternative");
            }
        },
        id);
}

} // namespace porytiles2

// Hash specializations for Id types (enables std::hash<PackableTile::Id>)
template <>
struct std::hash<porytiles2::PackableTile::HintId> {
    std::size_t operator()(const porytiles2::PackableTile::HintId &id) const noexcept
    {
        return std::hash<std::string>{}(id.name);
    }
};

template <>
struct std::hash<porytiles2::PackableTile::PrefilledPaletteId> {
    std::size_t operator()(const porytiles2::PackableTile::PrefilledPaletteId &id) const noexcept
    {
        return std::hash<std::size_t>{}(id.index());
    }
};

template <>
struct std::hash<porytiles2::PackableTile::RegularId> {
    std::size_t operator()(const porytiles2::PackableTile::RegularId &id) const noexcept
    {
        return std::hash<std::size_t>{}(id.index);
    }
};

template <>
struct std::hash<porytiles2::PackableTile::AnimId> {
    std::size_t operator()(const porytiles2::PackableTile::AnimId &id) const noexcept
    {
        // Hash combining via Boost's hash_combine formula: the magic constant 0x9e3779b9 is the golden ratio's
        // fractional part scaled to 32 bits, and the bit shifts spread bits to avoid collisions
        std::size_t seed = std::hash<std::string>{}(id.name);
        seed ^= std::hash<std::size_t>{}(id.subtile_index) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};
