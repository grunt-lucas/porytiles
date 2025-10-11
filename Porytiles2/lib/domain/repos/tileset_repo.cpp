#include "porytiles2/domain/repos/tileset_repo.hpp"

#include <set>
#include <string>

#include "fmt/format.h"

#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/repos/tileset_artifact_key_provider.hpp"
#include "porytiles2/domain/repos/tileset_artifact_reader.hpp"
#include "porytiles2/domain/repos/tileset_artifact_writer.hpp"
#include "porytiles2/domain/services/artifact_checksum_provider.hpp"
#include "porytiles2/templates/result.hpp"
#include "porytiles2/xcut/result/chainable_result.hpp"

namespace porytiles2 {

/*
 * TODO: we need better error handling, specifically the std::unexpected returns should be more descriptive
 */

ChainableResult<void> TilesetRepo::save(const Tileset &tileset) const
{
    using enum TilesetArtifact::Type;

    // Begin transaction for atomic writes
    if (auto result = writer_->begin_transaction(); !result) {
        return FormattableError{result.error()};
    }

    // Perform all write operations within the transaction

    // Porytiles assets
    // TODO: fill in the override and anim artifacts

    auto bottom_png_artifact = TilesetArtifact{bottom_png};
    auto bottom_png_key = key_provider_->key_for(tileset.name(), bottom_png_artifact);
    if (auto result = writer_->write(bottom_png_key, bottom_png_artifact, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{bottom_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    auto middle_png_artifact = TilesetArtifact{middle_png};
    auto middle_png_key = key_provider_->key_for(tileset.name(), middle_png_artifact);
    if (auto result = writer_->write(middle_png_key, middle_png_artifact, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{middle_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    auto top_png_artifact = TilesetArtifact{top_png};
    auto top_png_key = key_provider_->key_for(tileset.name(), top_png_artifact);
    if (auto result = writer_->write(top_png_key, top_png_artifact, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{top_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    auto attr_csv_artifact = TilesetArtifact{attributes_csv};
    auto attr_csv_key = key_provider_->key_for(tileset.name(), attr_csv_artifact);
    if (auto result = writer_->write(attr_csv_key, attr_csv_artifact, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{attr_csv_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    // Porymap assets
    // TODO: fill in the pal and anim artifacts

    auto metatiles_artifact = TilesetArtifact{metatiles_bin};
    auto metatiles_key = key_provider_->key_for(tileset.name(), metatiles_artifact);
    if (auto result = writer_->write(metatiles_key, metatiles_artifact, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{metatiles_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    auto attr_artifact = TilesetArtifact{metatile_attributes_bin};
    auto attr_key = key_provider_->key_for(tileset.name(), attr_artifact);
    if (auto result = writer_->write(attr_key, attr_artifact, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{attr_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    auto tiles_png_artifact = TilesetArtifact{tiles_png};
    auto tiles_png_key = key_provider_->key_for(tileset.name(), tiles_png_artifact);
    if (auto result = writer_->write(tiles_png_key, tiles_png_artifact, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{tiles_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    // TODO: don't hardcode 16 here
    constexpr int num_pals = 16;
    for (int i = 0; i < num_pals; i++) {
        const auto pal_key = key_provider_->key_for(tileset.name(), TilesetArtifact{pal_n, i});
        if (auto result = writer_->write(pal_key, TilesetArtifact{pal_n, i}, tileset); !result.has_value()) {
            std::ignore = writer_->rollback();
            auto failed = FormattableError{"{}: save failed", FormatParam{pal_key.key(), Style::bold}};
            return ChainableResult<void>{failed, result};
        }
    }

    // Commit all writes atomically
    if (auto result = writer_->commit(); !result.has_value()) {
        // Commit failed, attempt rollback (though it may not be necessary after failed commit)
        std::ignore = writer_->rollback();
        return ChainableResult<void>{FormattableError{"tileset commit failed"}, result};
    }

    // TODO: we should "clear" the stale contents of the tileset on disk after saving. That way, if the user e.g.
    // removed an anim, the stale Porymap version of the anim doesn't remain on disk and clutter the filesystem. Perhaps
    // this can be part of the tileset commit logic? We'll need some functionality in the writer implementation like
    // "clear_stale_contents" or something. This applies both ways, e.g. if we delete the anim on the porymap side,
    // then run an import, it should clear the anim from the porytiles side. I.e. if there is a Porymap anim on disk
    // that does not exist in the Porytiles component, clear it. If there is a Porytiles anim on disk that does not
    // exist in the Porymap component, clear it. Perhaps instead of auto-clearing, we can emit a diagnostic warning the
    // user that stale assets exist on disk?

    // Cache checksums after successful save
    const auto current_checksums = checksum_provider_->compute_tileset_artifact_checksums(tileset.name());
    const auto cache_result = checksum_provider_->cache_tileset_checksums(tileset.name(), current_checksums);
    if (!cache_result.has_value()) {
        return FormattableError{cache_result.error()};
    }
    return {};
}

ChainableResult<std::unique_ptr<Tileset>> TilesetRepo::load(const std::string &name) const
{
    using enum TilesetArtifact::Type;

    // Fail as late as possible
    bool fail_at_exit = false;

    // Confirm tileset exists.
    if (!exists(name)) {
        return FormattableError{"tileset '{}' does not exist", FormatParam{name, Style::bold}};
    }

    auto porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    auto tileset = std::make_unique<Tileset>(name, std::move(porytiles_component), std::move(porymap_component));

    // Porytiles assets

    const auto bottom_png_artifact = TilesetArtifact{bottom_png};
    const auto bottom_png_key = key_provider_->key_for(tileset->name(), bottom_png_artifact);
    if (key_provider_->artifact_exists(bottom_png_key)) {
        const auto result = reader_->read(*tileset, bottom_png_key, bottom_png_artifact);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{FormattableError{"failed to read bottom.png"}, result};
        }
    }

    const auto middle_png_artifact = TilesetArtifact{middle_png};
    const auto middle_png_key = key_provider_->key_for(tileset->name(), middle_png_artifact);
    if (key_provider_->artifact_exists(middle_png_key)) {
        const auto result = reader_->read(*tileset, middle_png_key, middle_png_artifact);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{FormattableError{"failed to read middle.png"}, result};
        }
    }

    const auto top_png_artifact = TilesetArtifact{top_png};
    const auto top_png_key = key_provider_->key_for(tileset->name(), top_png_artifact);
    if (key_provider_->artifact_exists(top_png_key)) {
        const auto result = reader_->read(*tileset, top_png_key, top_png_artifact);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{FormattableError{"failed to read top.png"}, result};
        }
    }

    const auto attr_csv_artifact = TilesetArtifact{attributes_csv};
    const auto attr_csv_key = key_provider_->key_for(tileset->name(), attr_csv_artifact);
    if (key_provider_->artifact_exists(attr_csv_key)) {
        const auto result = reader_->read(*tileset, attr_csv_key, attr_csv_artifact);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{FormattableError{"failed to read attributes.csv"}, result};
        }
    }
    else {
        // TODO: emit warning to user about missing attr csv
    }

    // TODO: don't hardcode 16 here
    constexpr int num_pal_overrides = 16;
    for (int i = 0; i < num_pal_overrides; i++) {
        const auto override_key = key_provider_->key_for(tileset->name(), TilesetArtifact{pal_override_n, i});
        if (key_provider_->artifact_exists(override_key)) {
            const auto result = reader_->read(*tileset, override_key, TilesetArtifact{pal_override_n, i});
            if (!result.has_value()) {
                return ChainableResult<std::unique_ptr<Tileset>>{
                    FormattableError{fmt::format("failed to read {}", override_key.key())}, result};
            }
        }
    }

    const std::set<std::string> porytiles_anims = key_provider_->discover_porytiles_anims(tileset->name());
    for (const auto &anim : porytiles_anims) {
        // Read frame 00.png
        auto frame_00_key = key_provider_->key_for(tileset->name(), TilesetArtifact{porytiles_anim_frame, anim, 0});
        if (!key_provider_->artifact_exists(frame_00_key)) {
            // TODO: emit validation error: missing required 00.png
            fail_at_exit = true;
            continue;
        }
        const auto frame_00_result =
            reader_->read(*tileset, frame_00_key, TilesetArtifact{porytiles_anim_frame, anim, 0});
        if (!frame_00_result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{fmt::format("failed to read {}", frame_00_key.key())}, frame_00_result};
        }

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
            const auto frame_n_result =
                reader_->read(*tileset, frame_n_key, TilesetArtifact{porytiles_anim_frame, anim, frame});
            if (!frame_n_result.has_value()) {
                return ChainableResult<std::unique_ptr<Tileset>>{
                    FormattableError{fmt::format("failed to read {}", frame_n_key.key())}, frame_n_result};
            }
            expected_frame++;
        }
    }

    // Porymap assets

    const auto metatiles_artifact = TilesetArtifact{metatiles_bin};
    const auto metatiles_key = key_provider_->key_for(tileset->name(), metatiles_artifact);
    if (!key_provider_->artifact_exists(metatiles_key)) {
        return FormattableError{"missing required porymap artifact metatiles.bin"};
    }
    const auto metatiles_result = reader_->read(*tileset, metatiles_key, metatiles_artifact);
    if (!metatiles_result.has_value()) {
        return ChainableResult<std::unique_ptr<Tileset>>{
            FormattableError{"failed to read metatiles.bin"}, metatiles_result};
    }

    const auto attr_artifact = TilesetArtifact{metatile_attributes_bin};
    const auto attr_key = key_provider_->key_for(tileset->name(), attr_artifact);
    if (!key_provider_->artifact_exists(attr_key)) {
        return FormattableError{"missing required porymap artifact metatile_attributes.bin"};
    }
    const auto attr_result = reader_->read(*tileset, attr_key, attr_artifact);
    if (!attr_result.has_value()) {
        return ChainableResult<std::unique_ptr<Tileset>>{
            FormattableError{"failed to read metatile_attributes.bin"}, attr_result};
    }

    const auto tiles_png_artifact = TilesetArtifact{tiles_png};
    const auto tiles_png_key = key_provider_->key_for(tileset->name(), tiles_png_artifact);
    if (!key_provider_->artifact_exists(tiles_png_key)) {
        return FormattableError{"missing required porymap artifact tiles.png"};
    }
    const auto tiles_png_result = reader_->read(*tileset, tiles_png_key, tiles_png_artifact);
    if (!tiles_png_result.has_value()) {
        return ChainableResult<std::unique_ptr<Tileset>>{
            FormattableError{"failed to read tiles.png"}, tiles_png_result};
    }

    // TODO: don't hardcode 16 here
    constexpr int num_pals = 16;
    for (int i = 0; i < num_pals; i++) {
        const auto pal_key = key_provider_->key_for(tileset->name(), TilesetArtifact{pal_n, i});
        if (!key_provider_->artifact_exists(pal_key)) {
            // TODO: emit validation error: missing required artifact {:02}.pal
            fail_at_exit = true;
            continue;
        }
        const auto pal_result = reader_->read(*tileset, pal_key, TilesetArtifact{pal_n, i});
        if (!pal_result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{fmt::format("failed to read {}", pal_key.key())}, pal_result};
        }
    }

    for (const std::set<std::string> porymap_anims = key_provider_->discover_porymap_anims(tileset->name());
         const auto &anim : porymap_anims) {
        // Read frame 00.png
        auto frame_00_key = key_provider_->key_for(tileset->name(), TilesetArtifact{porymap_anim_frame, anim, 0});
        if (!key_provider_->artifact_exists(frame_00_key)) {
            // TODO: emit validation error: missing required 00.png
            fail_at_exit = true;
            continue;
        }
        const auto frame_00_result =
            reader_->read(*tileset, frame_00_key, TilesetArtifact{porymap_anim_frame, anim, 0});
        if (!frame_00_result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{fmt::format("failed to read {}", frame_00_key.key())}, frame_00_result};
        }

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
            const auto frame_n_result =
                reader_->read(*tileset, frame_n_key, TilesetArtifact{porymap_anim_frame, anim, frame});
            if (!frame_n_result.has_value()) {
                return ChainableResult<std::unique_ptr<Tileset>>{
                    FormattableError{fmt::format("failed to read {}", frame_n_key.key())}, frame_n_result};
            }
            expected_frame++;
        }
    }

    if (fail_at_exit) {
        return FormattableError{"errors while loading tileset"};
    }

    return tileset;
}

bool TilesetRepo::exists(const std::string &name) const
{
    return key_provider_->tileset_exists(name);
}

} // namespace porytiles2
