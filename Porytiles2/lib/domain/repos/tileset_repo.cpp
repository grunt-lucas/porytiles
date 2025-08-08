#include "porytiles2/domain/repos/tileset_repo.hpp"

#include <set>
#include <string>

namespace porytiles2 {

/*
 * TODO: we need better error handling, specifically the std::unexpected returns should be more descriptive
 */

Result<void> TilesetRepo::save(const Tileset &tileset) const {
    using enum TilesetArtifact::Type;

    // Begin transaction for atomic writes
    if (auto result = writer_->begin_transaction(); !result) {
        return result;
    }

    // Perform all write operations within the transaction
    auto metatiles_key = key_provider_->key_for(tileset.name(), TilesetArtifact{metatiles_bin});
    if (auto result = writer_->write(metatiles_key, TilesetArtifact{metatiles_bin}, tileset); !result) {
        std::ignore = writer_->rollback();
        return result;
    }

    auto attr_key = key_provider_->key_for(tileset.name(), TilesetArtifact{metatile_attributes_bin});
    if (auto result = writer_->write(attr_key, TilesetArtifact{metatile_attributes_bin}, tileset); !result) {
        std::ignore = writer_->rollback();
        return result;
    }

    // TODO: fill in rest of the artifacts...

    // Commit all writes atomically
    if (auto result = writer_->commit(); !result) {
        // Commit failed, attempt rollback (though it may not be necessary after failed commit)
        std::ignore = writer_->rollback();
        return result;
    }

    // TODO: we should "clear" the stale contents of the tileset on disk after saving. That way, if the user e.g.
    // removed an anim, the stale Porymap version of the anim doesn't remain on disk and clutter the filesystem. Perhaps
    // this can be part of the tileset commit logic? E.g. if we implement the ProjectArtifactWriter using simple
    // filesystem directory move operations, this will be handled automatically since the new directory won't contain
    // any of the stale artifacts.

    // Cache checksums after successful save
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

    auto tileset = std::make_unique<Tileset>();
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

    // TODO: fill in rest of the required artifacts...

    // Now load optional artifacts, e.g. attributes csv, pal overrides, anims, etc.
    // attributes.csv
    if (const auto attr_csv_key = key_provider_->key_for(tileset->name(), TilesetArtifact{attributes_csv});
        key_provider_->exists(attr_csv_key)) {
        reader_->read(*tileset, attr_csv_key, TilesetArtifact{attributes_csv});
    } else {
        // TODO: emit warning to user about missing attr csv
    }

    // palette overrides
    constexpr int num_pals = 6; // TODO: Get this from Tileset class or config
    for (int i = 0; i < num_pals; i++) {
        if (auto override_key = key_provider_->key_for(tileset->name(), TilesetArtifact{override_n, i});
            key_provider_->exists(override_key)) {
            reader_->read(*tileset, override_key, TilesetArtifact{override_n, i});
        }
    }

    // porytiles anims
    for (const std::set<std::string> porytiles_anims = key_provider_->discover_porytiles_anims(tileset->name());
         const auto &anim : porytiles_anims) {
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

    // porymap anims
    for (const std::set<std::string> porymap_anims = key_provider_->discover_porymap_anims(tileset->name());
         const auto &anim : porymap_anims) {
        // Read frame 00.png
        auto frame_00_key = key_provider_->key_for(tileset->name(), TilesetArtifact{porymap_anim_frame, anim, 0});
        if (!key_provider_->exists(frame_00_key)) {
            // TODO: emit validation error: missing required 00.png
            fail_at_exit = true;
            continue;
        }
        reader_->read(*tileset, frame_00_key, TilesetArtifact{porymap_anim_frame, anim, 0});

        // Read the rest of the (optional) frames
        std::set<int> frames = key_provider_->discover_porymap_anim_frames(tileset->name(), anim);
        int expected_frame = 1;
        for (const auto frame : frames) {
            if (frame != expected_frame) {
                // TODO: emit validation error: frame {} did not match expected frame {}
                fail_at_exit = true;
            }
            auto frame_n_key =
                key_provider_->key_for(tileset->name(), TilesetArtifact{porymap_anim_frame, anim, frame});
            reader_->read(*tileset, frame_n_key, TilesetArtifact{porymap_anim_frame, anim, frame});
            expected_frame++;
        }
    }

    if (fail_at_exit) {
        return std::unexpected{"error loading tileset"};
    }

    return tileset;
}

} // namespace porytiles2
