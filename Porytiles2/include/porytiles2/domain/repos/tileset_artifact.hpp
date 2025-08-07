#pragma once

#include <optional>
#include <string>

namespace porytiles2 {

/**
 * @brief TODO: fill in
 */
class TilesetArtifact {
  public:
    enum class Type {
        bottom_png,
        middle_png,
        top_png,
        attributes_csv,
        porytiles_anim_key_frame,
        porytiles_anim_frame,
        override_n,
        metatiles_bin,
        metatile_attributes_bin,
        tiles_png,
        porymap_anim_frame,
        pal_n
    };

    explicit TilesetArtifact(const Type type) : type_{type}, name_{std::nullopt}, index_{std::nullopt} {}

    explicit TilesetArtifact(const Type type, std::string name)
        : type_{type}, name_{std::move(name)}, index_{std::nullopt} {}

    explicit TilesetArtifact(const Type type, int index) : type_{type}, name_{std::nullopt}, index_{index} {}

    explicit TilesetArtifact(const Type type, std::string name, int index)
        : type_{type}, name_{std::move(name)}, index_{index} {}

    [[nodiscard]] Type type() const {
        return type_;
    }

    [[nodiscard]] std::optional<std::string> name() const {
        return name_;
    }

    [[nodiscard]] std::optional<int> index() const {
        return index_;
    }

  private:
    Type type_;
    std::optional<std::string> name_;
    std::optional<int> index_;
};

} // namespace porytiles2
