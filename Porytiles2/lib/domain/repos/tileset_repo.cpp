#include "porytiles2/domain/repos/tileset_repo.hpp"

#include <set>
#include <string>

#include "porytiles2/domain/models/palette.hpp"
#include "porytiles2/domain/models/tileset.hpp"
#include "porytiles2/domain/repos/artifact_checksum_provider.hpp"
#include "porytiles2/domain/repos/tileset_artifact_key_provider.hpp"
#include "porytiles2/domain/repos/tileset_artifact_reader.hpp"
#include "porytiles2/domain/repos/tileset_artifact_writer.hpp"
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
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles_key, key_provider_->key_for_metatiles_bin(tileset.name()), "tileset save failed", void);
    if (auto result = writer_->write_metatiles_bin(metatiles_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{metatiles_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        attr_key, key_provider_->key_for_metatile_attributes_bin(tileset.name()), "tileset save failed", void);
    if (auto result = writer_->write_metatile_attributes_bin(attr_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{attr_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        tiles_png_key, key_provider_->key_for_tiles_png(tileset.name()), "tileset save failed", void);
    if (auto result = writer_->write_tiles_png(tiles_png_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{tiles_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    for (std::size_t i = 0; i < pal::num_pals; i++) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            pal_key, key_provider_->key_for_porymap_pal_n(tileset.name(), i), "tileset save failed", void);
        if (auto result = writer_->write_porymap_pal_n(pal_key, tileset, i); !result.has_value()) {
            std::ignore = writer_->rollback();
            auto failed = FormattableError{"{}: save failed", FormatParam{pal_key.key(), Style::bold}};
            return ChainableResult<void>{failed, result};
        }
    }

    for (const auto &porymap_anim : tileset.porymap_component().anims() | std::views::values) {
        for (std::size_t i = 0; i < porymap_anim.frame_count(); i++) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                frame_key,
                key_provider_->key_for_porymap_anim_frame(tileset.name(), porymap_anim.name(), i),
                "tileset save failed",
                void);
            if (auto result = writer_->write_porymap_anim_frame(frame_key, tileset, porymap_anim.name(), i);
                !result.has_value()) {
                std::ignore = writer_->rollback();
                auto failed = FormattableError{"{}: save failed", FormatParam{frame_key.key(), Style::bold}};
                return ChainableResult<void>{failed, result};
            }
        }
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        generated_anim_code_key,
        key_provider_->key_for_generated_anim_code(tileset.name()),
        "tileset save failed",
        void);
    if (auto result = writer_->write_generated_anim_code(generated_anim_code_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{generated_anim_code_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    /*
     * Porytiles artifacts
     */
    PT_TRY_ASSIGN_CHAIN_ERR(
        bottom_png_key, key_provider_->key_for_bottom_png(tileset.name()), "tileset save failed", void);
    if (auto result = writer_->write_bottom_png(bottom_png_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{bottom_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        middle_png_key, key_provider_->key_for_middle_png(tileset.name()), "tileset save failed", void);
    if (auto result = writer_->write_middle_png(middle_png_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{middle_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(top_png_key, key_provider_->key_for_top_png(tileset.name()), "tileset save failed", void);
    if (auto result = writer_->write_top_png(top_png_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{top_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        attr_csv_key, key_provider_->key_for_attributes_csv(tileset.name()), "tileset save failed", void);
    if (auto result = writer_->write_attributes_csv(attr_csv_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"{}: save failed", FormatParam{attr_csv_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    for (std::size_t i = 0; i < pal::num_pals; i++) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            porytiles_pal_key, key_provider_->key_for_porytiles_pal_n(tileset.name(), i), "tileset save failed", void);
        if (auto result = writer_->write_porytiles_pal_n(porytiles_pal_key, tileset, i); !result.has_value()) {
            std::ignore = writer_->rollback();
            auto failed = FormattableError{"{}: save failed", FormatParam{porytiles_pal_key.key(), Style::bold}};
            return ChainableResult<void>{failed, result};
        }
    }

    for (const auto &porytiles_anim : tileset.porytiles_component().anims() | std::views::values) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            key_frame_key,
            key_provider_->key_for_porytiles_anim_key_frame(tileset.name(), porytiles_anim.name()),
            "tileset save failed",
            void);
        if (auto result = writer_->write_porytiles_anim_key_frame(key_frame_key, tileset, porytiles_anim.name());
            !result.has_value()) {
            std::ignore = writer_->rollback();
            auto failed = FormattableError{"{}: save failed", FormatParam{key_frame_key.key(), Style::bold}};
            return ChainableResult<void>{failed, result};
        }

        for (std::size_t i = 0; i < porytiles_anim.frame_count(); i++) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                frame_key,
                key_provider_->key_for_porytiles_anim_frame(tileset.name(), porytiles_anim.name(), i),
                "tileset save failed",
                void);
            if (auto result = writer_->write_porytiles_anim_frame(frame_key, tileset, porytiles_anim.name(), i);
                !result.has_value()) {
                std::ignore = writer_->rollback();
                auto failed = FormattableError{"{}: save failed", FormatParam{frame_key.key(), Style::bold}};
                return ChainableResult<void>{failed, result};
            }
        }
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        anim_yaml_key, key_provider_->key_for_anim_yaml(tileset.name()), "tileset save failed", void);
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
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles_key,
        key_provider_->key_for_metatiles_bin(tileset->name()),
        "tileset load failed",
        std::unique_ptr<Tileset>);
    if (!key_provider_->artifact_exists(metatiles_key)) {
        return FormattableError{missing_required_artifact_msg, FormatParam{metatiles_key.key(), Style::bold}};
    }
    const auto metatiles_result = reader_->read_metatiles_bin(*tileset, metatiles_key);
    if (!metatiles_result.has_value()) {
        return ChainableResult<std::unique_ptr<Tileset>>{
            FormattableError{"failed to read artifact '{}'", FormatParam{metatiles_key.key(), Style::bold}},
            metatiles_result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        attr_key,
        key_provider_->key_for_metatile_attributes_bin(tileset->name()),
        "tileset load failed",
        std::unique_ptr<Tileset>);
    if (!key_provider_->artifact_exists(attr_key)) {
        return FormattableError{missing_required_artifact_msg, FormatParam{attr_key.key(), Style::bold}};
    }
    const auto attr_result = reader_->read_metatile_attributes_bin(*tileset, attr_key);
    if (!attr_result.has_value()) {
        return ChainableResult<std::unique_ptr<Tileset>>{
            FormattableError{"failed to read artifact '{}'", FormatParam{attr_key.key(), Style::bold}}, attr_result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        tiles_png_key,
        key_provider_->key_for_tiles_png(tileset->name()),
        "tileset load failed",
        std::unique_ptr<Tileset>);
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
        PT_TRY_ASSIGN_CHAIN_ERR(
            pal_key,
            key_provider_->key_for_porymap_pal_n(tileset->name(), i),
            "tileset load failed",
            std::unique_ptr<Tileset>);
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

    PT_TRY_ASSIGN_CHAIN_ERR(
        porymap_anims,
        key_provider_->discover_porymap_anims(tileset->name()),
        "tileset load failed",
        std::unique_ptr<Tileset>);
    for (const auto &porymap_anim : porymap_anims) {
        // Read the frames
        PT_TRY_ASSIGN_CHAIN_ERR(
            frames,
            key_provider_->discover_porymap_anim_frames(tileset->name(), porymap_anim),
            "tileset load failed",
            std::unique_ptr<Tileset>);
        for (const auto &frame : frames) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                frame_n_key,
                key_provider_->key_for_porymap_anim_frame(tileset->name(), porymap_anim, frame),
                "tileset load failed",
                std::unique_ptr<Tileset>);
            const auto frame_n_result =
                reader_->read_porymap_anim_frame(*tileset, frame_n_key, porymap_anim, std::to_string(frame));
            if (!frame_n_result.has_value()) {
                return ChainableResult<std::unique_ptr<Tileset>>{
                    FormattableError{"failed to read artifact '{}'", FormatParam{frame_n_key.key(), Style::bold}},
                    frame_n_result};
            }
        }
    }

    /*
     * TODO: ANIM: Refactor to keep AnimationCallbackInfo entirely in infra layer.
     *
     * CURRENT ISSUE:
     * AnimationCallbackInfo is an infra concept (C file paths, function names, project layout details), but it leaks
     * into the domain layer here. We fetch it via key_provider_ and pass it to reader_->read_anim_code().
     *
     * NAIVE FIX (won't work as-is):
     * Simply checking `!tileset->porymap_component().anims().empty()` instead of `callback_info_opt.has_value()`
     * doesn't solve the problem because AnimationCallbackInfo carries essential parsing context:
     *   - c_file_path(): WHERE to find the C code (generated_anim_code.h or tileset_anims.c)
     *   - callback_func_name(): WHAT function starts the callback chain
     *   - tileset_shorthand(): Used for function name matching during parsing
     *   - porytiles_managed(): Affects file path and function prefix expectations
     *
     * MISMATCH DETECTION (future enhancement):
     * There are two independent signals for "tileset has animations":
     *   1. Metadata-based: `.callback != NULL` in the tileset struct (headers.h)
     *   2. Frame-based: Animation directories exist with frame PNGs (0.png, 1.png, etc.)
     * These usually align, but mismatches indicate incomplete tilesets:
     *   - Callback exists but no frames: User defined anim code but hasn't created frame assets
     *   - Frames exist but no callback: User created frame assets but hasn't wired up anim code
     * We should warn the user about such mismatches rather than silently proceeding.
     *
     * RECOMMENDED REFACTORING APPROACH:
     * Move the callback info lookup INTO the reader (or a service the reader uses):
     *   1. Change read_anim_code() signature to not require AnimationCallbackInfo parameter
     *   2. Reader internally fetches callback info from metadata provider
     *   3. TilesetRepo either:
     *      a. Calls reader unconditionally (reader returns early if no anims), OR
     *      b. Checks discovered anims as optimization before calling
     *   4. Add mismatch detection: compare callback_info.has_value() vs !anims().empty()
     *      and emit warnings for inconsistencies
     *
     * RELATED TODOs:
     *   - animation_callback_info.hpp: "TODO: ANIM: this seems like a better fit for infra layer"
     *   - tileset_metadata_provider.hpp: "TODO: ANIM: all of this should be infra code"
     *   - tileset_metadata.hpp: "TODO: ANIM: a lot of this stuff belongs in the infra layer"
     *   - tileset_artifact_paths.hpp: "TODO: ANIM: this is a domain class but relies on infra concepts"
     */
    PT_TRY_ASSIGN_CHAIN_ERR(
        callback_info_opt,
        key_provider_->animation_callback_info_for(tileset->name()),
        "tileset load failed",
        std::unique_ptr<Tileset>);
    if (callback_info_opt.has_value()) {
        const auto result = reader_->read_anim_code(*tileset);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{
                    "{}: failed to read animation code",
                    FormatParam{callback_info_opt.value().c_file_path().string(), Style::bold}},
                result};
        }
    }

    /*
     * Porytiles artifacts
     */
    PT_TRY_ASSIGN_CHAIN_ERR(
        bottom_png_key,
        key_provider_->key_for_bottom_png(tileset->name()),
        "tileset load failed",
        std::unique_ptr<Tileset>);
    if (key_provider_->artifact_exists(bottom_png_key)) {
        const auto result = reader_->read_bottom_png(*tileset, bottom_png_key);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to read artifact '{}'", FormatParam{bottom_png_key.key(), Style::bold}},
                result};
        }
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        middle_png_key,
        key_provider_->key_for_middle_png(tileset->name()),
        "tileset load failed",
        std::unique_ptr<Tileset>);
    if (key_provider_->artifact_exists(middle_png_key)) {
        const auto result = reader_->read_middle_png(*tileset, middle_png_key);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to read artifact '{}'", FormatParam{middle_png_key.key(), Style::bold}},
                result};
        }
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        top_png_key, key_provider_->key_for_top_png(tileset->name()), "tileset load failed", std::unique_ptr<Tileset>);
    if (key_provider_->artifact_exists(top_png_key)) {
        const auto result = reader_->read_top_png(*tileset, top_png_key);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to read artifact '{}'", FormatParam{top_png_key.key(), Style::bold}}, result};
        }
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        attr_csv_key,
        key_provider_->key_for_attributes_csv(tileset->name()),
        "tileset load failed",
        std::unique_ptr<Tileset>);
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
        PT_TRY_ASSIGN_CHAIN_ERR(
            override_key,
            key_provider_->key_for_porytiles_pal_n(tileset->name(), i),
            "tileset load failed",
            std::unique_ptr<Tileset>);
        if (key_provider_->artifact_exists(override_key)) {
            const auto result = reader_->read_porytiles_pal_n(*tileset, override_key, i);
            if (!result.has_value()) {
                return ChainableResult<std::unique_ptr<Tileset>>{
                    FormattableError{"failed to read artifact '{}'", FormatParam{override_key.key(), Style::bold}},
                    result};
            }
        }
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        porytiles_anims,
        key_provider_->discover_porytiles_anims(tileset->name()),
        "tileset load failed",
        std::unique_ptr<Tileset>);
    for (const auto &porytiles_anim : porytiles_anims) {
        // Read key.png
        PT_TRY_ASSIGN_CHAIN_ERR(
            key_frame_key,
            key_provider_->key_for_porytiles_anim_key_frame(tileset->name(), porytiles_anim),
            "tileset load failed",
            std::unique_ptr<Tileset>);
        if (!key_provider_->artifact_exists(key_frame_key)) {
            diag_->error(
                missing_required_artifact_tag,
                format_->format(missing_required_artifact_msg, FormatParam{key_frame_key.key(), Style::bold}));
            fail_at_exit = true;
            continue;
        }
        const auto key_frame_result = reader_->read_porytiles_anim_key_frame(*tileset, key_frame_key, porytiles_anim);
        if (!key_frame_result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to read artifact '{}'", FormatParam{key_frame_key.key(), Style::bold}},
                key_frame_result};
        }

        // Read frame 0.png
        PT_TRY_ASSIGN_CHAIN_ERR(
            frame_0_key,
            key_provider_->key_for_porytiles_anim_frame(tileset->name(), porytiles_anim, 0),
            "tileset load failed",
            std::unique_ptr<Tileset>);
        if (!key_provider_->artifact_exists(frame_0_key)) {
            diag_->error(
                missing_required_artifact_tag,
                format_->format(missing_required_artifact_msg, FormatParam{frame_0_key.key(), Style::bold}));
            fail_at_exit = true;
            continue;
        }
        const auto frame_0_result = reader_->read_porytiles_anim_frame(*tileset, frame_0_key, porytiles_anim, 0);
        if (!frame_0_result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"failed to read artifact '{}'", FormatParam{frame_0_key.key(), Style::bold}},
                frame_0_result};
        }

        // Read the rest of the (optional) frames
        PT_TRY_ASSIGN_CHAIN_ERR(
            frames,
            key_provider_->discover_porytiles_anim_frames(tileset->name(), porytiles_anim),
            "tileset load failed",
            std::unique_ptr<Tileset>);
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
            PT_TRY_ASSIGN_CHAIN_ERR(
                frame_n_key,
                key_provider_->key_for_porytiles_anim_frame(tileset->name(), porytiles_anim, frame),
                "tileset load failed",
                std::unique_ptr<Tileset>);
            const auto frame_n_result =
                reader_->read_porytiles_anim_frame(*tileset, frame_n_key, porytiles_anim, frame);
            if (!frame_n_result.has_value()) {
                return ChainableResult<std::unique_ptr<Tileset>>{
                    FormattableError{"failed to read artifact '{}'", FormatParam{frame_n_key.key(), Style::bold}},
                    frame_n_result};
            }
            expected_frame++;
        }
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        anim_yaml_key,
        key_provider_->key_for_anim_yaml(tileset->name()),
        "tileset load failed",
        std::unique_ptr<Tileset>);
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
