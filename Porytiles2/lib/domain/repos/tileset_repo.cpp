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

ChainableResult<void> TilesetRepo::save(const Tileset &tileset) const
{
    // Begin transaction for atomic writes
    if (auto result = writer_->begin_transaction(); !result.has_value()) {
        return ChainableResult<void>{FormattableError{"Tileset begin transaction failed."}, result};
    }

    // Perform all write operations within the transaction

    /*
     * Porymap artifacts
     */
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles_key, key_provider_->key_for_metatiles_bin(tileset.name()), "Tileset save failed.", void);
    if (auto result = writer_->write_metatiles_bin(metatiles_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"Save failed for '{}'.", FormatParam{metatiles_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        attr_key, key_provider_->key_for_metatile_attributes_bin(tileset.name()), "Tileset save failed.", void);
    if (auto result = writer_->write_metatile_attributes_bin(attr_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"Save failed for '{}'.", FormatParam{attr_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        tiles_png_key, key_provider_->key_for_tiles_png(tileset.name()), "Tileset save failed.", void);
    if (auto result = writer_->write_tiles_png(tiles_png_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"Save failed for '{}'.", FormatParam{tiles_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    for (std::size_t i = 0; i < pal::num_pals; i++) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            pal_key, key_provider_->key_for_porymap_pal_n(tileset.name(), i), "Tileset save failed.", void);
        if (auto result = writer_->write_porymap_pal_n(pal_key, tileset, i); !result.has_value()) {
            std::ignore = writer_->rollback();
            auto failed = FormattableError{"Save failed for '{}'.", FormatParam{pal_key.key(), Style::bold}};
            return ChainableResult<void>{failed, result};
        }
    }

    for (const auto &porymap_anim : tileset.porymap_component().anims() | std::views::values) {
        for (const auto &frame : porymap_anim.frames_values()) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                frame_key,
                key_provider_->key_for_porymap_anim_frame(tileset.name(), porymap_anim.name(), frame.frame_name()),
                "Tileset save failed.",
                void);
            if (auto result =
                    writer_->write_porymap_anim_frame(frame_key, tileset, porymap_anim.name(), frame.frame_name());
                !result.has_value()) {
                std::ignore = writer_->rollback();
                auto failed = FormattableError{"Save failed for '{}'.", FormatParam{frame_key.key(), Style::bold}};
                return ChainableResult<void>{failed, result};
            }
        }
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        generated_anim_code_key,
        key_provider_->key_for_porymap_anim_params(tileset.name()),
        "Tileset save failed.",
        void);
    if (auto result = writer_->write_porymap_anim_params(generated_anim_code_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed =
            FormattableError{"Save failed for '{}'.", FormatParam{generated_anim_code_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    /*
     * Porytiles artifacts
     */
    PT_TRY_ASSIGN_CHAIN_ERR(
        bottom_png_key, key_provider_->key_for_bottom_png(tileset.name()), "Tileset save failed.", void);
    if (auto result = writer_->write_bottom_png(bottom_png_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"Save failed for '{}'.", FormatParam{bottom_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        middle_png_key, key_provider_->key_for_middle_png(tileset.name()), "Tileset save failed.", void);
    if (auto result = writer_->write_middle_png(middle_png_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"Save failed for '{}'.", FormatParam{middle_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(top_png_key, key_provider_->key_for_top_png(tileset.name()), "Tileset save failed.", void);
    if (auto result = writer_->write_top_png(top_png_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"Save failed for '{}'.", FormatParam{top_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        attr_csv_key, key_provider_->key_for_attributes_csv(tileset.name()), "Tileset save failed.", void);
    if (auto result = writer_->write_attributes_csv(attr_csv_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"Save failed for '{}'.", FormatParam{attr_csv_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    for (std::size_t i = 0; i < pal::num_pals; i++) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            porytiles_pal_key, key_provider_->key_for_porytiles_pal_n(tileset.name(), i), "Tileset save failed.", void);
        if (auto result = writer_->write_porytiles_pal_n(porytiles_pal_key, tileset, i); !result.has_value()) {
            std::ignore = writer_->rollback();
            auto failed = FormattableError{"Save failed for '{}'.", FormatParam{porytiles_pal_key.key(), Style::bold}};
            return ChainableResult<void>{failed, result};
        }
    }

    for (const auto &porytiles_anim : tileset.porytiles_component().anims() | std::views::values) {
        // Save key frame (only present for automatic/hybrid frame linking)
        if (porytiles_anim.has_key_frame()) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                key_frame_key,
                key_provider_->key_for_porytiles_anim_frame(
                    tileset.name(), porytiles_anim.name(), porytiles_anim.key_frame().frame_name()),
                "Tileset save failed.",
                void);
            if (auto result = writer_->write_porytiles_anim_frame(
                    key_frame_key, tileset, porytiles_anim.name(), porytiles_anim.key_frame().frame_name());
                !result.has_value()) {
                std::ignore = writer_->rollback();
                auto failed = FormattableError{"Save failed for '{}'.", FormatParam{key_frame_key.key(), Style::bold}};
                return ChainableResult<void>{failed, result};
            }
        }

        // Save regular frames
        for (const auto &frame : porytiles_anim.frames_values()) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                frame_key,
                key_provider_->key_for_porytiles_anim_frame(tileset.name(), porytiles_anim.name(), frame.frame_name()),
                "Tileset save failed.",
                void);
            if (auto result =
                    writer_->write_porytiles_anim_frame(frame_key, tileset, porytiles_anim.name(), frame.frame_name());
                !result.has_value()) {
                std::ignore = writer_->rollback();
                auto failed = FormattableError{"Save failed for '{}'.", FormatParam{frame_key.key(), Style::bold}};
                return ChainableResult<void>{failed, result};
            }
        }
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        anim_json_key, key_provider_->key_for_porytiles_anim_params(tileset.name()), "Tileset save failed.", void);
    if (auto result = writer_->write_porytiles_anim_params(anim_json_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"Save failed for '{}'.", FormatParam{anim_json_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    // Commit all writes atomically
    if (auto result = writer_->commit(); !result.has_value()) {
        // Commit failed, attempt rollback (though it may not be necessary after failed commit)
        std::ignore = writer_->rollback();
        return ChainableResult<void>{FormattableError{"Tileset commit failed."}, result};
    }

    // Cache checksums after successful save
    PT_TRY_ASSIGN_CHAIN_ERR(
        artifact_keys,
        key_provider_->get_all_artifact_keys(tileset.name()),
        "Failed to get artifact keys for tileset save.",
        void);
    const auto current_checksums = checksum_provider_->compute_tileset_artifact_checksums(artifact_keys);
    const auto cache_result = checksum_provider_->cache_tileset_checksums(tileset.name(), current_checksums);
    if (!cache_result.has_value()) {
        return FormattableError{cache_result.error()};
    }
    return {};
}

ChainableResult<std::unique_ptr<Tileset>> TilesetRepo::load(const std::string &name) const
{
    constexpr auto missing_required_artifact_tag = "missing-required-artifact";
    constexpr auto missing_required_artifact_msg = "Missing required artifact: '{}'.";
    constexpr auto missing_optional_artifact_tag = "missing-optional-artifact";
    constexpr auto missing_optional_artifact_msg = "Missing optional artifact: '{}'.";

    // Fail as late as possible
    bool fail_at_exit = false;

    // Confirm tileset exists.
    if (!exists(name)) {
        return FormattableError{"Tileset '{}' does not exist.", FormatParam{name, Style::bold}};
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
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
        std::unique_ptr<Tileset>);
    if (!key_provider_->artifact_exists(metatiles_key)) {
        return FormattableError{missing_required_artifact_msg, FormatParam{metatiles_key.key(), Style::bold}};
    }
    const auto metatiles_result = reader_->read_metatiles_bin(*tileset, metatiles_key);
    if (!metatiles_result.has_value()) {
        return ChainableResult<std::unique_ptr<Tileset>>{
            FormattableError{"Failed to read artifact: '{}'.", FormatParam{metatiles_key.key(), Style::bold}},
            metatiles_result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        attr_key,
        key_provider_->key_for_metatile_attributes_bin(tileset->name()),
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
        std::unique_ptr<Tileset>);
    if (!key_provider_->artifact_exists(attr_key)) {
        return FormattableError{missing_required_artifact_msg, FormatParam{attr_key.key(), Style::bold}};
    }
    const auto attr_result = reader_->read_metatile_attributes_bin(*tileset, attr_key);
    if (!attr_result.has_value()) {
        return ChainableResult<std::unique_ptr<Tileset>>{
            FormattableError{"Failed to read artifact: '{}'.", FormatParam{attr_key.key(), Style::bold}}, attr_result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        tiles_png_key,
        key_provider_->key_for_tiles_png(tileset->name()),
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
        std::unique_ptr<Tileset>);
    if (!key_provider_->artifact_exists(tiles_png_key)) {
        return FormattableError{missing_required_artifact_msg, FormatParam{tiles_png_key.key(), Style::bold}};
    }
    const auto tiles_png_result = reader_->read_tiles_png(*tileset, tiles_png_key);
    if (!tiles_png_result.has_value()) {
        return ChainableResult<std::unique_ptr<Tileset>>{
            FormattableError{"Failed to read artifact '{}'.", FormatParam{tiles_png_key.key(), Style::bold}},
            tiles_png_result};
    }

    for (std::size_t i = 0; i < pal::num_pals; i++) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            pal_key,
            key_provider_->key_for_porymap_pal_n(tileset->name(), i),
            diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
            std::unique_ptr<Tileset>);
        if (!key_provider_->artifact_exists(pal_key)) {
            diag_->error(
                missing_required_artifact_tag, missing_required_artifact_msg, FormatParam{pal_key.key(), Style::bold});
            fail_at_exit = true;
            continue;
        }
        const auto pal_result = reader_->read_porymap_pal_n(*tileset, pal_key, i);
        if (!pal_result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"Failed to read artifact '{}'.", FormatParam{pal_key.key(), Style::bold}}, pal_result};
        }
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        porymap_anim_params_key,
        key_provider_->key_for_porymap_anim_params(tileset->name()),
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
        std::unique_ptr<Tileset>);

    PT_TRY_ASSIGN_CHAIN_ERR(
        porymap_anims,
        key_provider_->discover_porymap_anims(tileset->name()),
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
        std::unique_ptr<Tileset>);

    for (const auto &porymap_anim_name : porymap_anims) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            frames,
            key_provider_->discover_porymap_anim_frames(tileset->name(), porymap_anim_name),
            diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
            std::unique_ptr<Tileset>);

        std::vector<std::pair<std::string, ArtifactKey>> frames_keys{};
        for (const auto &frame_name : frames) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                frame_key,
                key_provider_->key_for_porymap_anim_frame(tileset->name(), porymap_anim_name, frame_name),
                diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
                std::unique_ptr<Tileset>);

            if (!key_provider_->artifact_exists(frame_key)) {
                return FormattableError{missing_required_artifact_msg, FormatParam{frame_key.key(), Style::bold}};
            }
            frames_keys.emplace_back(frame_name, frame_key);
        }

        const auto anim_result =
            reader_->read_porymap_anim(*tileset, porymap_anim_name, porymap_anim_params_key, frames_keys);
        if (!anim_result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"Failed to read animation '{}'.", FormatParam{porymap_anim_name, Style::bold}},
                anim_result};
        }
    }

    /*
     * Porytiles artifacts
     */
    PT_TRY_ASSIGN_CHAIN_ERR(
        bottom_png_key,
        key_provider_->key_for_bottom_png(tileset->name()),
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
        std::unique_ptr<Tileset>);
    if (key_provider_->artifact_exists(bottom_png_key)) {
        const auto result = reader_->read_bottom_png(*tileset, bottom_png_key);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"Failed to read artifact '{}'.", FormatParam{bottom_png_key.key(), Style::bold}},
                result};
        }
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        middle_png_key,
        key_provider_->key_for_middle_png(tileset->name()),
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
        std::unique_ptr<Tileset>);
    if (key_provider_->artifact_exists(middle_png_key)) {
        const auto result = reader_->read_middle_png(*tileset, middle_png_key);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"Failed to read artifact '{}'.", FormatParam{middle_png_key.key(), Style::bold}},
                result};
        }
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        top_png_key,
        key_provider_->key_for_top_png(tileset->name()),
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
        std::unique_ptr<Tileset>);
    if (key_provider_->artifact_exists(top_png_key)) {
        const auto result = reader_->read_top_png(*tileset, top_png_key);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"Failed to read artifact '{}'.", FormatParam{top_png_key.key(), Style::bold}}, result};
        }
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        attr_csv_key,
        key_provider_->key_for_attributes_csv(tileset->name()),
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
        std::unique_ptr<Tileset>);
    if (key_provider_->artifact_exists(attr_csv_key)) {
        const auto result = reader_->read_attributes_csv(*tileset, attr_csv_key);
        if (!result.has_value()) {
            return ChainableResult<std::unique_ptr<Tileset>>{
                FormattableError{"Failed to read artifact '{}'.", FormatParam{attr_csv_key.key(), Style::bold}},
                result};
        }
    }
    else {
        diag_->warning(
            missing_optional_artifact_tag, missing_optional_artifact_msg, FormatParam{attr_csv_key.key(), Style::bold});
        diag_->warning_note(missing_optional_artifact_tag, "All attributes will receive default or inferred values.");
    }

    for (std::size_t i = 0; i < pal::num_pals; i++) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            override_key,
            key_provider_->key_for_porytiles_pal_n(tileset->name(), i),
            diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
            std::unique_ptr<Tileset>);
        if (key_provider_->artifact_exists(override_key)) {
            const auto result = reader_->read_porytiles_pal_n(*tileset, override_key, i);
            if (!result.has_value()) {
                return ChainableResult<std::unique_ptr<Tileset>>{
                    FormattableError{"Failed to read artifact '{}'.", FormatParam{override_key.key(), Style::bold}},
                    result};
            }
        }
    }

    // Load Porytiles animations using unified read method
    PT_TRY_ASSIGN_CHAIN_ERR(
        porytiles_params_key,
        key_provider_->key_for_porytiles_anim_params(tileset->name()),
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
        std::unique_ptr<Tileset>);

    if (key_provider_->artifact_exists(porytiles_params_key)) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            porytiles_anims,
            key_provider_->discover_porytiles_anims(tileset->name()),
            diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
            std::unique_ptr<Tileset>);

        for (const auto &anim_name : porytiles_anims) {
            // TODO: don't hardcode key here?
            PT_TRY_ASSIGN_CHAIN_ERR(
                key_frame_key,
                key_provider_->key_for_porytiles_anim_frame(tileset->name(), anim_name, "key"),
                diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
                std::unique_ptr<Tileset>);

            /*
             * Here, we check to make sure the key frame is present. If not, we'll throw an error. In the next step,
             * we'll discover the actually existing frames to pass to the anim reader. Since we already validated that
             * the key frame is present, we know it will get passed in.
             */
            if (!key_provider_->artifact_exists(key_frame_key)) {
                // TODO: throw error here and continue, like below
                return FormattableError{missing_required_artifact_msg, FormatParam{key_frame_key.key(), Style::bold}};
            }

            PT_TRY_ASSIGN_CHAIN_ERR(
                frame_names,
                key_provider_->discover_porytiles_anim_frames(tileset->name(), anim_name),
                diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
                std::unique_ptr<Tileset>);

            // Build frame_keys vector for unified read
            std::vector<std::pair<std::string, ArtifactKey>> frame_keys{};
            bool anim_has_missing_frames = false;
            for (const auto &frame_name : frame_names) {
                PT_TRY_ASSIGN_CHAIN_ERR(
                    frame_key,
                    key_provider_->key_for_porytiles_anim_frame(tileset->name(), anim_name, frame_name),
                    diag_->formatter().format(
                        "Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}),
                    std::unique_ptr<Tileset>);

                if (!key_provider_->artifact_exists(frame_key)) {
                    diag_->error(
                        missing_required_artifact_tag,
                        missing_required_artifact_msg,
                        FormatParam{frame_key.key(), Style::bold});
                    fail_at_exit = true;
                    anim_has_missing_frames = true;
                    continue;
                }
                frame_keys.emplace_back(frame_name, frame_key);
            }

            if (anim_has_missing_frames) {
                continue; // Skip this animation if it has missing frames
            }

            // Load complete animation with unified method
            const auto result =
                reader_->read_porytiles_anim(*tileset, anim_name, porytiles_params_key, key_frame_key, frame_keys);
            if (!result.has_value()) {
                return ChainableResult<std::unique_ptr<Tileset>>{
                    FormattableError{"Failed to load Porytiles animation '{}'.", FormatParam{anim_name, Style::bold}},
                    result};
            }
        }
    }

    if (fail_at_exit) {
        return FormattableError{"Errors while loading tileset '{}'.", FormatParam{tileset->name(), Style::bold}};
    }

    return tileset;
}

bool TilesetRepo::exists(const std::string &tileset_name) const
{
    return metadata_provider_->exists(tileset_name);
}

} // namespace porytiles2
