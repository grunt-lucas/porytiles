#include "porytiles2/domain/repos/tileset_repo.hpp"

#include <set>
#include <string>

namespace porytiles2 {

Result<void> TilesetRepo::save(const Tileset &tileset) const {
    // auto metatiles_key = key_provider_.key_for(tileset.name(), TilesetArtifact{metatiles_bin});
    // auto attr_key = key_provider_.key_for(tileset.name(), TilesetArtifact{metatile_attr_bin});
    //
    // writer_.write(metatiles_key, TilesetArtifact{metatiles_bin}, tileset);
    // writer_.write(attr_key, TilesetArtifact{metatile_attr_bin}, tileset);

    // TODO: we should "clear" the stale contents of the tileset on disk after saving. That way, if the user e.g.
    // removed an anim, the stale Porymap version of the anim doesn't remain on disk and clutter the filesystem.

    // if (auto save_result = save_tileset(tileset); !save_result.has_value()) {
    //     return save_result;
    // }

    const auto current_checksums = metadata_provider_->compute_artifact_checksums(tileset.name());
    return metadata_provider_->cache_checksums(tileset.name(), current_checksums);
}

Result<std::unique_ptr<Tileset>> TilesetRepo::load(const std::string &name) const {
    using enum TilesetArtifact::Type;

    // Fail as late as possible
    bool fail_at_exit = false;

    // Confirm tileset exists.
    if (!exists(name)) {
        return std::unexpected{"does not exist"};
    }

    // Init tileset using the virtual factory method
    auto tileset = create_empty_tileset();
    tileset->name(name);
    tileset->porytiles_component(std::make_unique<PorytilesTilesetComponent>());
    tileset->porymap_component(std::make_unique<PorymapTilesetComponent>());

    // Load artifacts from required keys first. We can check if they exist before performing a read op.
    const auto metatiles_key = key_provider_->key_for(tileset->name(), TilesetArtifact{metatiles_bin});
    if (!key_provider_->exists(metatiles_key)) {
        return std::unexpected{"missing required porymap artifact metatiles.bin"};
    }
    reader_->read(*tileset, metatiles_key, TilesetArtifact{metatiles_bin});

    const auto attr_key = key_provider_->key_for(tileset->name(), TilesetArtifact{metatile_attributes_bin});
    if (!key_provider_->exists(attr_key)) {
        return std::unexpected{"missing required porymap artifact metatile_attributes.bin"};
    }
    reader_->read(*tileset, attr_key, TilesetArtifact{metatile_attributes_bin});

    auto bottom_png_key = key_provider_->key_for(tileset->name(), TilesetArtifact{bottom_png});
    if (!key_provider_->exists(bottom_png_key)) {
        return std::unexpected{"missing required porytiles artifact bottom.png"};
    }
    // TODO: in this case, bottom.png does not map directly onto a field of PorytilesTilesetComponent, how to handle?
    reader_->read(*tileset, bottom_png_key, TilesetArtifact{bottom_png});

    // More artifacts...

    // Now load optional artifacts, e.g. attributes csv, pal overrides, anims, etc.
    // attributes.csv
    auto attr_csv_key = key_provider_->key_for(tileset->name(), TilesetArtifact{attributes_csv});
    if (key_provider_->exists(attr_csv_key)) {
        reader_->read(*tileset, attr_csv_key, TilesetArtifact{attributes_csv});
    } else {
        // Emit warning to user about missing attr csv
    }

    // palette overrides
    constexpr int num_pals = 6; // TODO: Get this from Tileset class or config
    for (int i = 0; i < num_pals; i++) {
        auto override_key = key_provider_->key_for(tileset->name(), TilesetArtifact{override_n, i});
        if (key_provider_->exists(override_key)) {
            reader_->read(*tileset, override_key, TilesetArtifact{override_n, i});
        }
    }

    // porytiles anims
    std::set<std::string> porytiles_anims = key_provider_->discover_porytiles_anims(tileset->name());
    for (const auto &anim : porytiles_anims) {
        // Read key frame
        auto key_frame_key = key_provider_->key_for(tileset->name(), TilesetArtifact{porytiles_anim_key_frame, anim});
        if (!key_provider_->exists(key_frame_key)) {
            // TODO: emit validation error: missing required key frame
            fail_at_exit = true;
            continue;
        }
        reader_->read(*tileset, key_frame_key, TilesetArtifact{porytiles_anim_key_frame, anim});

        // Read frame 00.png
        auto frame_00_key = key_provider_->key_for(tileset->name(), TilesetArtifact{porytiles_anim_frame, anim, 0});
        if (!key_provider_->exists(frame_00_key)) {
            // TODO: emit validation error: missing required 00.png
            fail_at_exit = true;
            continue;
        }
        reader_->read(*tileset, frame_00_key, TilesetArtifact{porytiles_anim_frame, anim, 0});

        // Read the rest of the (optional) frames
        std::set<int> frames = key_provider_->discover_porytiles_anim_frames(tileset->name(), anim);
        int expected_frame = 1;
        for (const auto frame : frames) {
            if (frame != expected_frame) {
                // TODO: emit validation error: frame {} did not match expected frame {}
                fail_at_exit = true;
            }
            auto frame_n_key =
                key_provider_->key_for(tileset->name(), TilesetArtifact{porytiles_anim_frame, anim, frame});
            reader_->read(*tileset, frame_n_key, TilesetArtifact{porytiles_anim_frame, anim, frame});
            expected_frame++;
        }
    }

    // porymap anims would be the same, but without key frame logic

    if (fail_at_exit) {
        return std::unexpected{"error loading tileset"};
    }

    return tileset;
}

} // namespace porytiles2
