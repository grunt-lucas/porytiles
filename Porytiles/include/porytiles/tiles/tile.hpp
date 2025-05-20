#pragma once

#include <array>

#include "../panic/panic.hpp"
#include "./tile_metadata.hpp"

namespace porytiles {

constexpr std::size_t kTileSideLength = 8;
constexpr std::size_t kTileSize = kTileSideLength * kTileSideLength;

/// @brief Represents a single tile,
/// which can store pixel data and type-specific metadata.
///
/// @details
/// A Tile object encapsulates a pixel data array of a user-defined type.
/// Each tile has a TileType that determines the kind of metadata it holds.
/// The metadata is stored in a std::variant,
/// allowing for different metadata structures depending on the TileType.
/// This class provides methods to access the tile's type
/// and its associated metadata in a type-safe manner.
template <typename P> class Tile {
    std::array<P, kTileSize> pix_;
    TileType type_;
    TileMetadata metadata_;

  protected:
    [[nodiscard]] const std::array<P, kTileSize> &pix() const {
        return pix_;
    }

  public:
    explicit Tile(const TileType t) : pix_{}, type_(t) {
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

    [[nodiscard]] virtual bool IsTransparent(const P &transparency) const {
        return std::ranges::all_of(pix(), [=](const auto &pixel) { return pixel == transparency; });
    }

    [[nodiscard]] P At(std::size_t i) const {
        if (i >= kTileSize) {
            Panic(fmt::format("Index {} out of bounds", i));
        }
        return pix_[i];
    }

    [[nodiscard]] P At(std::size_t row, std::size_t col) const {
        if (row >= kTileSideLength) {
            Panic(fmt::format("Row index {} out of bounds", row));
        }
        if (col >= kTileSideLength) {
            Panic(fmt::format("Col index {} out of bounds", col));
        }
        return pix_[row * kTileSideLength + col];
    }

    void Set(std::size_t i, const P &p) {
        if (i >= kTileSize) {
            Panic(fmt::format("Index {} out of bounds", i));
        }
        pix_[i] = p;
    }

    void Set(std::size_t row, std::size_t col, const P &p) {
        if (row >= kTileSideLength) {
            Panic(fmt::format("Row index {} out of bounds", row));
        }
        if (col >= kTileSideLength) {
            Panic(fmt::format("Col index {} out of bounds", col));
        }
        pix_[row * kTileSideLength + col] = p;
    }
};

} // namespace porytiles
