#include "porytiles2/domain/repos/tileset_repo.hpp"

#include <set>
#include <string>

#include "fmt/format.h"

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/rgba32.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/repos/tileset_artifact_key_provider.hpp"
#include "porytiles2/domain/repos/tileset_artifact_reader.hpp"
#include "porytiles2/domain/repos/tileset_artifact_writer.hpp"
#include "porytiles2/domain/services/artifact_checksum_provider.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"

namespace porytiles2 {

/*
 * TODO: we need better error handling, specifically the std::unexpected returns should be more descriptive
 */

ChainableResult<void> TilesetRepo::save(const Tileset &tileset) const
{
    // Begin transaction for atomic writes
    if (auto result = writer_->begin_transaction(); !result.has_value()) {
        return ChainableResult<void>{FormattableError{"tileset begin transaction failed"}, result};
    }

    // Perform all write operations within the transaction

    /*
     * Porymap artifacts
     */
    auto metatiles_key = key_provider_->key_for_metatiles_bin(tileset.name());
    if (auto result = writer_->write_metatiles_bin(metatiles_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{metatiles_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    auto attr_key = key_provider_->key_for_metatile_attributes_bin(tileset.name());
    if (auto result = writer_->write_metatile_attributes_bin(attr_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{attr_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    auto tiles_png_key = key_provider_->key_for_tiles_png(tileset.name());
    if (auto result = writer_->write_tiles_png(tiles_png_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{tiles_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    for (std::size_t i = 0; i < pal::num_pals; i++) {
        const auto pal_key = key_provider_->key_for_porymap_pal_n(tileset.name(), i);
        if (auto result = writer_->write_porymap_pal_n(pal_key, tileset, i); !result.has_value()) {
            std::ignore = writer_->rollback();
            auto failed = FormattableError{"{}: save failed", FormatParam{pal_key.key(), Style::bold}};
            return ChainableResult<void>{failed, result};
        }
    }

    for (const auto &anim : tileset.porymap_component().anims() | std::views::values) {
        for (std::size_t i = 0; i < anim.frame_count(); i++) {
            const auto frame_key = key_provider_->key_for_porymap_anim_frame(tileset.name(), anim.name(), i);
            if (auto result = writer_->write_porymap_anim_frame(frame_key, tileset, anim.name(), i);
                !result.has_value()) {
                std::ignore = writer_->rollback();
                auto failed = FormattableError{"{}: save failed", FormatParam{frame_key.key(), Style::bold}};
                return ChainableResult<void>{failed, result};
            }
        }
    }

    auto generated_anim_code_key = key_provider_->key_for_generated_anim_code(tileset.name());
    if (auto result = writer_->write_generated_anim_code(generated_anim_code_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{generated_anim_code_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    /*
     * Porytiles artifacts
     */
    auto bottom_png_key = key_provider_->key_for_bottom_png(tileset.name());
    if (auto result = writer_->write_bottom_png(bottom_png_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{bottom_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    auto middle_png_key = key_provider_->key_for_middle_png(tileset.name());
    if (auto result = writer_->write_middle_png(middle_png_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{middle_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    auto top_png_key = key_provider_->key_for_top_png(tileset.name());
    if (auto result = writer_->write_top_png(top_png_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{top_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    auto attr_csv_key = key_provider_->key_for_attributes_csv(tileset.name());
    if (auto result = writer_->write_attributes_csv(attr_csv_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{attr_csv_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    for (std::size_t i = 0; i < pal::num_pals; i++) {
        const auto pal_key = key_provider_->key_for_porytiles_pal_n(tileset.name(), i);
        if (auto result = writer_->write_porytiles_pal_n(pal_key, tileset, i); !result.has_value()) {
            std::ignore = writer_->rollback();
            auto failed = FormattableError{"{}: save failed", FormatParam{pal_key.key(), Style::bold}};
            return ChainableResult<void>{failed, result};
        }
    }

    for (const auto &anim : tileset.porytiles_component().anims() | std::views::values) {
        const auto key_frame_key = key_provider_->key_for_porytiles_anim_key_frame(tileset.name(), anim.name());
        if (auto result = writer_->write_porytiles_anim_key_frame(key_frame_key, tileset, anim.name());
            !result.has_value()) {
            std::ignore = writer_->rollback();
            auto failed = FormattableError{"{}: save failed", FormatParam{key_frame_key.key(), Style::bold}};
            return ChainableResult<void>{failed, result};
        }

        for (std::size_t i = 0; i < anim.frame_count(); i++) {
            const auto frame_key = key_provider_->key_for_porytiles_anim_frame(tileset.name(), anim.name(), i);
            if (auto result = writer_->write_porytiles_anim_frame(frame_key, tileset, anim.name(), i);
                !result.has_value()) {
                std::ignore = writer_->rollback();
                auto failed = FormattableError{"{}: save failed", FormatParam{frame_key.key(), Style::bold}};
                return ChainableResult<void>{failed, result};
            }
        }
    }

    auto anim_yaml_key = key_provider_->key_for_anim_yaml(tileset.name());
    if (auto result = writer_->write_anim_yaml(anim_yaml_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{anim_yaml_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
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
    constexpr auto missing_required_artifact_tag = "missing-required-artifact";
    constexpr auto missing_required_artifact_msg = "missing required artifact: '{}'";
    constexpr auto missing_optional_artifact_tag = "missing-optional-artifact";
    constexpr auto missing_optional_artifact_msg = "missing optional artifact: '{}'";

    // Fail as late as possible
    bool fail_at_exit = false;

    // Confirm tileset exists.
    if (!exists(name)) {
        return FormattableError{"tileset '{}' does not exist", FormatParam{name, Style::bold}};
    }

    auto porytiles_component = std::make_unique<PorytilesTilesetComponent>();
    auto porymap_component = std::make_unique<PorymapTilesetComponent>();
    auto tileset = std::make_unique<Tileset>(name, std::move(porytiles_component), std::move(porymap_component));

    /*
     * Porymap artifacts
     */
    const auto metatiles_key = key_provider_->key_for_metatiles_bin(tileset->name());
    if (!key_provider_->artifact_exists(metatiles_key)) {
        return FormattableError{missing_required_artifact_msg, FormatParam{metatiles_key.key(), Style::bold}};
    }
    const auto metatiles_result = reader_->read_metatiles_bin(*tileset, metatiles_key);
    if (!metatiles_result.has_value()) {
        return ChainableResult<std::unique_ptr<Tileset>>{
            FormattableError{"failed to read artifact '{}'", FormatParam{metatiles_key.key(), Style::bold}},
            metatiles_result};
    }

    const auto attr_key = key_provider_->key_for_metatile_attributes_bin(tileset->name());
    if (!key_provider_->artifact_exists(attr_key)) {
        return FormattableError{missing_required_artifact_msg, FormatParam{attr_key.key(), Style::bold}};
    }
    const auto attr_result = reader_->read_metatile_attributes_bin(*tileset, attr_key);
    if (!attr_result.has_value()) {
        return ChainableResult<std::unique_ptr<Tileset>>{
            FormattableError{"failed to read artifact '{}'", FormatParam{attr_key.key(), Style::bold}}, attr_result};
    }

    const auto tiles_png_key = key_provider_->key_for_tiles_png(tileset->name());
    if (!key_provider_->artifact_exists(tiles_png_key)) {
        return FormattableError{missing_required_artifact_msg, FormatParam{tiles_png_key.key(), Style::bold}};
    }
    const auto tiles_png_result = reader_->read_tiles_png(*tileset, tiles_png_key);
    if (!tiles_png_result.has_value()) {
        return ChainableResult<std::unique_ptr<Tileset>>{
            FormattableError{"failed to read artifact '{}'", FormatParam{tiles_png_key.key(), Style::bold}},
            tiles_png_result};
    }

    for (std::size_t i = 0; i < pal::num_pals; i++) {
        const auto pal_key = key_provider_->key_for_porymap_pal_n(tileset->name(), i);
        if (!key_provider_->artifact_exists(pal_key)) {
            diag_->error(
                missing_required_artifact_tag,
                format_->format(missing_required_artifact_msg, FormatParam{pal_key.key(), Style::bold}));
            fail_at_exit = true;
            continue;
        }
        const auto pal_result = reader_->read_porymap_pal_n(*tileset, pal_key, i);
        if (!pal_result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to read artifact '{}'", FormatParam{pal_key.key(), Style::bold}}, pal_result};
        }
    }

    const std::set<std::string> porymap_anims = key_provider_->discover_porymap_anims(tileset->name());
    for (const auto &anim : porymap_anims) {
        // Read frame 0.png
        auto frame_0_key = key_provider_->key_for_porymap_anim_frame(tileset->name(), anim, 0);
        if (!key_provider_->artifact_exists(frame_0_key)) {
            diag_->error(
                missing_required_artifact_tag,
                format_->format(missing_required_artifact_msg, FormatParam{frame_0_key.key(), Style::bold}));
            fail_at_exit = true;
            continue;
        }
        const auto frame_0_result = reader_->read_porymap_anim_frame(*tileset, frame_0_key, anim, 0);
        if (!frame_0_result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to read artifact '{}'", FormatParam{frame_0_key.key(), Style::bold}},
                frame_0_result};
        }

        // Read the rest of the (optional) frames
        std::set<int> frames = key_provider_->discover_porymap_anim_frames(tileset->name(), anim);
        int expected_frame = 1;
        for (const auto frame : frames) {
            if (frame != expected_frame) {
                diag_->error(
                    "out-of-order-frame-index",
                    format_->format(
                        "found frame '{}' but expected '{}'",
                        FormatParam{frame, Style::bold},
                        FormatParam{expected_frame, Style::bold}));
                // TODO: add a note here to explain this more
                fail_at_exit = true;
            }
            auto frame_n_key = key_provider_->key_for_porymap_anim_frame(tileset->name(), anim, frame);
            const auto frame_n_result = reader_->read_porymap_anim_frame(*tileset, frame_n_key, anim, frame);
            if (!frame_n_result.has_value()) {
                return ChainableResult<std::unique_ptr<Tileset>>{
                    FormattableError{"failed to read artifact '{}'", FormatParam{frame_n_key.key(), Style::bold}},
                    frame_n_result};
            }
            expected_frame++;
        }
    }

    const auto generated_anim_code_key = key_provider_->key_for_generated_anim_code(tileset->name());
    if (key_provider_->artifact_exists(generated_anim_code_key)) {
        const auto result = reader_->read_generated_anim_code(*tileset, generated_anim_code_key, porymap_anims);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{
                    "failed to read artifact '{}'", FormatParam{generated_anim_code_key.key(), Style::bold}},
                result};
        }
    }
    else {
        // TODO: this needs to be relative to project root, add handling to the key provider
        ArtifactKey vanilla_anim_code_key{"src/tileset_anims.c"};
        const auto result = reader_->read_vanilla_anim_code(*tileset, vanilla_anim_code_key, porymap_anims);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{
                    "{}: failed to read vanilla anim code", FormatParam{vanilla_anim_code_key.key(), Style::bold}},
                result};
        }
    }

    /*
     * Porytiles artifacts
     */
    const auto bottom_png_key = key_provider_->key_for_bottom_png(tileset->name());
    if (key_provider_->artifact_exists(bottom_png_key)) {
        const auto result = reader_->read_bottom_png(*tileset, bottom_png_key);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to read artifact '{}'", FormatParam{bottom_png_key.key(), Style::bold}},
                result};
        }
    }

    const auto middle_png_key = key_provider_->key_for_middle_png(tileset->name());
    if (key_provider_->artifact_exists(middle_png_key)) {
        const auto result = reader_->read_middle_png(*tileset, middle_png_key);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to read artifact '{}'", FormatParam{middle_png_key.key(), Style::bold}},
                result};
        }
    }

    const auto top_png_key = key_provider_->key_for_top_png(tileset->name());
    if (key_provider_->artifact_exists(top_png_key)) {
        const auto result = reader_->read_top_png(*tileset, top_png_key);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to read artifact '{}'", FormatParam{top_png_key.key(), Style::bold}}, result};
        }
    }

    const auto attr_csv_key = key_provider_->key_for_attributes_csv(tileset->name());
    if (key_provider_->artifact_exists(attr_csv_key)) {
        const auto result = reader_->read_attributes_csv(*tileset, attr_csv_key);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to read artifact '{}'", FormatParam{attr_csv_key.key(), Style::bold}}, result};
        }
    }
    else {
        diag_->warning(
            missing_optional_artifact_tag,
            format_->format(missing_optional_artifact_msg, FormatParam{attr_csv_key.key(), Style::bold}));
        diag_->note(missing_optional_artifact_tag, "all attributes will receive default or inferred values");
    }

    for (std::size_t i = 0; i < pal::num_pals; i++) {
        const auto override_key = key_provider_->key_for_porytiles_pal_n(tileset->name(), i);
        if (key_provider_->artifact_exists(override_key)) {
            const auto result = reader_->read_porytiles_pal_n(*tileset, override_key, i);
            if (!result.has_value()) {
                return ChainableResult<std::unique_ptr<Tileset>>{
                    FormattableError{"failed to read artifact '{}'", FormatParam{override_key.key(), Style::bold}},
                    result};
            }
        }
    }

    const std::set<std::string> porytiles_anims = key_provider_->discover_porytiles_anims(tileset->name());
    for (const auto &anim : porytiles_anims) {
        // Read key.png
        auto key_frame_key = key_provider_->key_for_porytiles_anim_key_frame(tileset->name(), anim);
        if (!key_provider_->artifact_exists(key_frame_key)) {
            diag_->error(
                missing_required_artifact_tag,
                format_->format(missing_required_artifact_msg, FormatParam{key_frame_key.key(), Style::bold}));
            fail_at_exit = true;
            continue;
        }
        const auto key_frame_result = reader_->read_porytiles_anim_key_frame(*tileset, key_frame_key, anim);
        if (!key_frame_result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to read artifact '{}'", FormatParam{key_frame_key.key(), Style::bold}},
                key_frame_result};
        }

        // Read frame 0.png
        auto frame_0_key = key_provider_->key_for_porytiles_anim_frame(tileset->name(), anim, 0);
        if (!key_provider_->artifact_exists(frame_0_key)) {
            diag_->error(
                missing_required_artifact_tag,
                format_->format(missing_required_artifact_msg, FormatParam{frame_0_key.key(), Style::bold}));
            fail_at_exit = true;
            continue;
        }
        const auto frame_0_result = reader_->read_porytiles_anim_frame(*tileset, frame_0_key, anim, 0);
        if (!frame_0_result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to read artifact '{}'", FormatParam{frame_0_key.key(), Style::bold}},
                frame_0_result};
        }

        // Read the rest of the (optional) frames
        std::set<int> frames = key_provider_->discover_porytiles_anim_frames(tileset->name(), anim);
        int expected_frame = 1;
        for (const auto frame : frames) {
            if (frame != expected_frame) {
                diag_->error(
                    "out-of-order-frame-index",
                    format_->format(
                        "found frame '{}' but expected '{}'",
                        FormatParam{frame, Style::bold},
                        FormatParam{expected_frame, Style::bold}));
                // TODO: add a note here to explain this more
                fail_at_exit = true;
            }
            auto frame_n_key = key_provider_->key_for_porytiles_anim_frame(tileset->name(), anim, frame);
            const auto frame_n_result = reader_->read_porytiles_anim_frame(*tileset, frame_n_key, anim, frame);
            if (!frame_n_result.has_value()) {
                return ChainableResult<std::unique_ptr<Tileset>>{
                    FormattableError{"failed to read artifact '{}'", FormatParam{frame_n_key.key(), Style::bold}},
                    frame_n_result};
            }
            expected_frame++;
        }
    }

    const auto anim_yaml_key = key_provider_->key_for_anim_yaml(tileset->name());
    if (key_provider_->artifact_exists(anim_yaml_key)) {
        const auto result = reader_->read_anim_yaml(*tileset, anim_yaml_key);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to read artifact '{}'", FormatParam{anim_yaml_key.key(), Style::bold}},
                result};
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
