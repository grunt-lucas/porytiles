#pragma once

#include <map>
#include <set>
#include <string>

#include "porytiles/domain/repos/artifact_key.hpp"
#include "porytiles/domain/repos/tileset_artifact_key_provider.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/string_utils.hpp"

namespace porytiles {

/// @brief An in-memory TilesetArtifactKeyProvider for tests.
///
/// @details
/// Keys are short fixed strings: Porymap artifacts live under `bin/` and Porytiles artifacts under `src/`.
/// A test fills in @c existing and the two animation maps to describe what the backing store holds.
/// The tileset name is ignored, so one instance models one tileset.
class FakeTilesetArtifactKeyProvider final : public TilesetArtifactKeyProvider {
  public:
    /// The keys of the artifacts present in the backing store
    std::set<ArtifactKey> existing{};
    /// Porymap animation name -> frame names found in the backing store
    std::map<std::string, std::set<std::string>> porymap_anims{};
    /// Porytiles animation name -> frame names found in the backing store
    std::map<std::string, std::set<std::string>> porytiles_anims{};

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_metatiles_bin(const std::string &) const override
    {
        return ArtifactKey{"bin/metatiles.bin"};
    }

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_metatile_attributes_bin(const std::string &) const override
    {
        return ArtifactKey{"bin/metatile_attributes.bin"};
    }

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_tiles_png(const std::string &) const override
    {
        return ArtifactKey{"bin/tiles.png"};
    }

    [[nodiscard]] ChainableResult<ArtifactKey>
    key_for_porymap_palette_n(const std::string &, std::size_t index) const override
    {
        return ArtifactKey{"bin/palettes/" + palette_filename(index)};
    }

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_porymap_anim_frame(
        const std::string &, const std::string &anim_name, const std::string &frame_name) const override
    {
        return ArtifactKey{"bin/anim/" + anim_name + "/" + frame_name + ".png"};
    }

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_porymap_anim_params(const std::string &) const override
    {
        return ArtifactKey{"generated/anim_code.h"};
    }

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_bottom_png(const std::string &) const override
    {
        return ArtifactKey{"src/bottom.png"};
    }

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_middle_png(const std::string &) const override
    {
        return ArtifactKey{"src/middle.png"};
    }

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_top_png(const std::string &) const override
    {
        return ArtifactKey{"src/top.png"};
    }

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_attributes_csv(const std::string &) const override
    {
        return ArtifactKey{"src/attributes.csv"};
    }

    [[nodiscard]] ChainableResult<ArtifactKey>
    key_for_porytiles_palette_n(const std::string &, std::size_t index) const override
    {
        return ArtifactKey{"src/palettes/" + palette_filename(index)};
    }

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_porytiles_anim_frame(
        const std::string &, const std::string &anim_name, const std::string &frame_name) const override
    {
        return ArtifactKey{"src/anim/" + anim_name + "/" + frame_name + ".png"};
    }

    [[nodiscard]] ChainableResult<ArtifactKey> key_for_porytiles_anim_params(const std::string &) const override
    {
        return ArtifactKey{"src/anim/anim.json"};
    }

    [[nodiscard]] bool artifact_exists(const ArtifactKey &key) const override
    {
        return existing.contains(key);
    }

    [[nodiscard]] ChainableResult<std::set<std::string>> discover_porymap_anims(const std::string &) const override
    {
        return anim_names(porymap_anims);
    }

    [[nodiscard]] ChainableResult<std::set<std::string>>
    discover_porymap_anim_frames(const std::string &, const std::string &anim_name) const override
    {
        return frame_names(porymap_anims, anim_name);
    }

    [[nodiscard]] ChainableResult<std::set<std::string>> discover_porytiles_anims(const std::string &) const override
    {
        return anim_names(porytiles_anims);
    }

    [[nodiscard]] ChainableResult<std::set<std::string>>
    discover_porytiles_anim_frames(const std::string &, const std::string &anim_name) const override
    {
        return frame_names(porytiles_anims, anim_name);
    }

  private:
    [[nodiscard]] static std::set<std::string> anim_names(const std::map<std::string, std::set<std::string>> &anims)
    {
        std::set<std::string> names{};
        for (const auto &[name, frames] : anims) {
            names.insert(name);
        }
        return names;
    }

    [[nodiscard]] static std::set<std::string>
    frame_names(const std::map<std::string, std::set<std::string>> &anims, const std::string &anim_name)
    {
        return anims.contains(anim_name) ? anims.at(anim_name) : std::set<std::string>{};
    }
};

} // namespace porytiles
