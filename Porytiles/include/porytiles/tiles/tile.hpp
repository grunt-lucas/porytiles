#pragma once

#include <array>

#include "../panic/panic.hpp"
#include "./tile_metadata.hpp"

namespace porytiles {

constexpr std::size_t kTileSideLength = 8;
constexpr std::size_t kTileSize = kTileSideLength * kTileSideLength;

using TileMetadata = std::variant<std::monostate, FreeMetadata, LayeredMetadata>;

/// @brief Represents a single tile,
/// which can store pixel data and type-specific metadata.
///
/// @details
/// A Tile object encapsulates an array of pixel data (pix_)
/// of a user-defined type T.
/// Each tile has a TileType that determines the kind of metadata it holds.
/// The metadata is stored in a std::variant (metadata_),
/// allowing for different metadata structures depending on the TileType.
/// This class provides methods to access the tile's type
/// and its associated metadata in a type-safe manner.
template <typename T> class Tile {
    std::array<T, kTileSize> pix_;
    TileType type_;
    TileMetadata metadata_;

  public:
    explicit Tile(const TileType t) : type_(t) {
        switch (t) {
        case TileType::kFree:
            metadata_ = FreeMetadata{};
            break;
        case TileType::kLayered:
            metadata_ = LayeredMetadata{};
            break;
        default:
            metadata_ = std::monostate{};
        }
    }

    [[nodiscard]] TileType type() const noexcept {
        return type_;
    }

    /// @brief Gets a constant reference to the tile's typed metadata.
    /// @tparam M The expected type of the metadata to retrieve.
    /// @return A constant reference to the metadata object of type `M`.
    template <typename M> [[nodiscard]] const M &metadata() const {
        if (auto *m = std::get_if<M>(&metadata_)) {
            return *m;
        }
        Panic("Metadata std::variant did not contain expected type");
    }

    /// @brief Gets a mutable reference to the tile's typed metadata.
    /// @tparam M The expected type of the metadata to retrieve.
    /// @return A mutable reference to the metadata object of type `M`.
    template <typename M> [[nodiscard]] M &metadata() {
        if (auto *m = std::get_if<M>(&metadata_)) {
            return *m;
        }
        Panic("Metadata std::variant did not contain expected type");
    }
};

} // namespace porytiles