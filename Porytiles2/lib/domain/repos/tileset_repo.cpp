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

    // Porytiles assets
    // TODO: fill in the override and anim artifacts

    auto bottom_png_artifact = TilesetArtifact{bottom_png};
    auto bottom_png_key = key_provider_->key_for(tileset.name(), bottom_png_artifact);
    if (auto result = writer_->write(bottom_png_key, bottom_png_artifact, tileset); !result) {
        std::ignore = writer_->rollback();
        return result;
    }

    auto middle_png_artifact = TilesetArtifact{middle_png};
    auto middle_png_key = key_provider_->key_for(tileset.name(), middle_png_artifact);
    if (auto result = writer_->write(middle_png_key, middle_png_artifact, tileset); !result) {
        std::ignore = writer_->rollback();
        return result;
    }

    auto top_png_artifact = TilesetArtifact{top_png};
    auto top_png_key = key_provider_->key_for(tileset.name(), top_png_artifact);
    if (auto result = writer_->write(top_png_key, top_png_artifact, tileset); !result) {
        std::ignore = writer_->rollback();
        return result;
    }

    auto attr_csv_artifact = TilesetArtifact{attributes_csv};
    auto attr_csv_key = key_provider_->key_for(tileset.name(), attr_csv_artifact);
    if (auto result = writer_->write(attr_csv_key, attr_csv_artifact, tileset); !result) {
        std::ignore = writer_->rollback();
        return result;
    }

    // Porymap assets
    // TODO: fill in the pal and anim artifacts

    auto metatiles_artifact = TilesetArtifact{metatiles_bin};
    auto metatiles_key = key_provider_->key_for(tileset.name(), metatiles_artifact);
    if (auto result = writer_->write(metatiles_key, metatiles_artifact, tileset); !result) {
        std::ignore = writer_->rollback();
        return result;
    }

    auto attr_artifact = TilesetArtifact{metatile_attributes_bin};
    auto attr_key = key_provider_->key_for(tileset.name(), attr_artifact);
    if (auto result = writer_->write(attr_key, attr_artifact, tileset); !result) {
        std::ignore = writer_->rollback();
        return result;
    }

    auto tiles_png_artifact = TilesetArtifact{tiles_png};
    auto tiles_png_key = key_provider_->key_for(tileset.name(), tiles_png_artifact);
    if (auto result = writer_->write(tiles_png_key, tiles_png_artifact, tileset); !result) {
        std::ignore = writer_->rollback();
        return result;
    }

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
    // any of the stale artifacts. We'll just need to make sure we don't clobber anything that is present in one of the
    // tileset components but isn't an explicit result of a de/compilation operation, e.g. pal overrides, pal hints, PLA
    // files, etc.

    // Cache checksums after successful save
    const auto current_checksums = checksum_provider_->compute_artifact_checksums(tileset.name());
    return checksum_provider_->cache_checksums(tileset.name(), current_checksums);
}

Result<std::unique_ptr<Tileset>> TilesetRepo::load(const std::string &name) const {
    using enum TilesetArtifact::Type;

    // Fail as late as possible
    bool fail_at_exit = false;

    // Confirm tileset exists.
    if (!exists(name)) {
        return std::unexpected{"does not exist"};
    }

    auto porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    auto tileset = std::make_unique<Tileset>(name, std::move(porytiles_component), std::move(porymap_component));

    // Porytiles assets

    const auto bottom_png_artifact = TilesetArtifact{bottom_png};
    const auto bottom_png_key = key_provider_->key_for(tileset->name(), bottom_png_artifact);
    if (key_provider_->exists(bottom_png_key)) {
        reader_->read(*tileset, bottom_png_key, bottom_png_artifact);
    }

    const auto middle_png_artifact = TilesetArtifact{middle_png};
    const auto middle_png_key = key_provider_->key_for(tileset->name(), middle_png_artifact);
    if (key_provider_->exists(middle_png_key)) {
        reader_->read(*tileset, middle_png_key, middle_png_artifact);
    }

    const auto top_png_artifact = TilesetArtifact{top_png};
    const auto top_png_key = key_provider_->key_for(tileset->name(), top_png_artifact);
    if (key_provider_->exists(top_png_key)) {
        reader_->read(*tileset, top_png_key, top_png_artifact);
    }

    const auto attr_csv_artifact = TilesetArtifact{attributes_csv};
    const auto attr_csv_key = key_provider_->key_for(tileset->name(), attr_csv_artifact);
    if (key_provider_->exists(attr_csv_key)) {
        reader_->read(*tileset, attr_csv_key, attr_csv_artifact);
    } else {
        // TODO: emit warning to user about missing attr csv
    }

    // TODO: don't hardcode this num_pal_overrides value
    constexpr int num_pal_overrides = 16;
    for (int i = 0; i < num_pal_overrides; i++) {
        const auto override_key = key_provider_->key_for(tileset->name(), TilesetArtifact{pal_override_n, i});
        if (key_provider_->exists(override_key)) {
            reader_->read(*tileset, override_key, TilesetArtifact{pal_override_n, i});
        }
    }

    const std::set<std::string> porytiles_anims = key_provider_->discover_porytiles_anims(tileset->name());
    for (const auto &anim : porytiles_anims) {
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

    // Porymap assets

    const auto metatiles_artifact = TilesetArtifact{metatiles_bin};
    const auto metatiles_key = key_provider_->key_for(tileset->name(), metatiles_artifact);
    if (!key_provider_->exists(metatiles_key)) {
        return std::unexpected{"missing required porymap artifact metatiles.bin"};
    }
    reader_->read(*tileset, metatiles_key, metatiles_artifact);

    const auto attr_artifact = TilesetArtifact{metatile_attributes_bin};
    const auto attr_key = key_provider_->key_for(tileset->name(), attr_artifact);
    if (!key_provider_->exists(attr_key)) {
        return std::unexpected{"missing required porymap artifact metatile_attributes.bin"};
    }
    reader_->read(*tileset, attr_key, attr_artifact);

    const auto tiles_png_artifact = TilesetArtifact{tiles_png};
    const auto tiles_png_key = key_provider_->key_for(tileset->name(), tiles_png_artifact);
    if (!key_provider_->exists(tiles_png_key)) {
        return std::unexpected{"missing required porymap artifact tiles.png"};
    }
    reader_->read(*tileset, attr_key, tiles_png_artifact);

    // TODO: don't hardcode this num_pals value
    constexpr int num_pals = 16;
    for (int i = 0; i < num_pals; i++) {
        const auto pal_key = key_provider_->key_for(tileset->name(), TilesetArtifact{pal_n, i});
        if (!key_provider_->exists(pal_key)) {
            return std::unexpected{fmt::format("missing required artifact {:02}.pal", i)};
        }
        reader_->read(*tileset, pal_key, TilesetArtifact{pal_n, i});
    }

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

bool TilesetRepo::exists(const std::string &name) const {
    return key_provider_->tileset_exists(name);
}

} // namespace porytiles2
