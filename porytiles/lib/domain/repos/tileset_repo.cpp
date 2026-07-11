#include "porytiles/domain/repos/tileset_repo.hpp"

#include <optional>
#include <set>
#include <string>

#include "porytiles/domain/models/palette.hpp"
#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/domain/repos/artifact_checksum_provider.hpp"
#include "porytiles/domain/repos/tileset_artifact_key_provider.hpp"
#include "porytiles/domain/repos/tileset_artifact_reader.hpp"
#include "porytiles/domain/repos/tileset_artifact_writer.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"

namespace porytiles {

ChainableResult<void> TilesetRepo::save(const Tileset &tileset) const
{
    // Begin transaction for atomic writes
    PT_TRY_CALL_CHAIN_ERR(writer_->begin_transaction(), void, "Tileset begin transaction failed.");

    // Perform all write operations within the transaction

    // Porymap artifacts
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles_key, key_provider_->key_for_metatiles_bin(tileset.name()), void, "Tileset save failed.");
    if (auto result = writer_->write_metatiles_bin(metatiles_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"Save failed for '{}'.", FormatParam{metatiles_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        attr_key, key_provider_->key_for_metatile_attributes_bin(tileset.name()), void, "Tileset save failed.");
    if (auto result = writer_->write_metatile_attributes_bin(attr_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"Save failed for '{}'.", FormatParam{attr_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        tiles_png_key, key_provider_->key_for_tiles_png(tileset.name()), void, "Tileset save failed.");
    if (auto result = writer_->write_tiles_png(tiles_png_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"Save failed for '{}'.", FormatParam{tiles_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    for (std::size_t i = 0; i < pal::num_pals; i++) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            pal_key, key_provider_->key_for_porymap_pal_n(tileset.name(), i), void, "Tileset save failed.");
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
                void,
                "Tileset save failed.");
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
        void,
        "Tileset save failed.");
    if (auto result = writer_->write_porymap_anim_params(generated_anim_code_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed =
            FormattableError{"Save failed for '{}'.", FormatParam{generated_anim_code_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    // Porytiles artifacts
    PT_TRY_ASSIGN_CHAIN_ERR(
        bottom_png_key, key_provider_->key_for_bottom_png(tileset.name()), void, "Tileset save failed.");
    if (auto result = writer_->write_bottom_png(bottom_png_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"Save failed for '{}'.", FormatParam{bottom_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        middle_png_key, key_provider_->key_for_middle_png(tileset.name()), void, "Tileset save failed.");
    if (auto result = writer_->write_middle_png(middle_png_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"Save failed for '{}'.", FormatParam{middle_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(top_png_key, key_provider_->key_for_top_png(tileset.name()), void, "Tileset save failed.");
    if (auto result = writer_->write_top_png(top_png_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"Save failed for '{}'.", FormatParam{top_png_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        attr_csv_key, key_provider_->key_for_attributes_csv(tileset.name()), void, "Tileset save failed.");
    if (auto result = writer_->write_attributes_csv(attr_csv_key, tileset); !result.has_value()) {
        std::ignore = writer_->rollback();
        auto failed = FormattableError{"Save failed for '{}'.", FormatParam{attr_csv_key.key(), Style::bold}};
        return ChainableResult<void>{failed, result};
    }

    for (std::size_t i = 0; i < pal::num_pals; i++) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            porytiles_pal_key, key_provider_->key_for_porytiles_pal_n(tileset.name(), i), void, "Tileset save failed.");
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
                void,
                "Tileset save failed.");
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
                void,
                "Tileset save failed.");
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
        anim_json_key, key_provider_->key_for_porytiles_anim_params(tileset.name()), void, "Tileset save failed.");
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
        void,
        "Failed to get artifact keys for tileset save.");
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

    // Porymap artifacts
    PT_TRY_ASSIGN_CHAIN_ERR(
        metatiles_key,
        key_provider_->key_for_metatiles_bin(tileset->name()),
        std::unique_ptr<Tileset>,
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));
    if (!key_provider_->artifact_exists(metatiles_key)) {
        return FormattableError{missing_required_artifact_msg, FormatParam{metatiles_key.key(), Style::bold}};
    }
    PT_TRY_CALL_CHAIN_ERR(
        reader_->read_metatiles_bin(*tileset, metatiles_key),
        std::unique_ptr<Tileset>,
        "Failed to read artifact: '{}'.",
        FormatParam(metatiles_key.key(), Style::bold));

    PT_TRY_ASSIGN_CHAIN_ERR(
        attr_key,
        key_provider_->key_for_metatile_attributes_bin(tileset->name()),
        std::unique_ptr<Tileset>,
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));
    if (!key_provider_->artifact_exists(attr_key)) {
        return FormattableError{missing_required_artifact_msg, FormatParam{attr_key.key(), Style::bold}};
    }
    PT_TRY_CALL_CHAIN_ERR(
        reader_->read_metatile_attributes_bin(*tileset, attr_key),
        std::unique_ptr<Tileset>,
        "Failed to read artifact: '{}'.",
        FormatParam(attr_key.key(), Style::bold));

    PT_TRY_ASSIGN_CHAIN_ERR(
        tiles_png_key,
        key_provider_->key_for_tiles_png(tileset->name()),
        std::unique_ptr<Tileset>,
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));
    if (!key_provider_->artifact_exists(tiles_png_key)) {
        return FormattableError{missing_required_artifact_msg, FormatParam{tiles_png_key.key(), Style::bold}};
    }
    PT_TRY_CALL_CHAIN_ERR(
        reader_->read_tiles_png(*tileset, tiles_png_key),
        std::unique_ptr<Tileset>,
        "Failed to read artifact '{}'.",
        FormatParam(tiles_png_key.key(), Style::bold));

    for (std::size_t i = 0; i < pal::num_pals; i++) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            pal_key,
            key_provider_->key_for_porymap_pal_n(tileset->name(), i),
            std::unique_ptr<Tileset>,
            diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));
        if (!key_provider_->artifact_exists(pal_key)) {
            diag_->error(
                missing_required_artifact_tag, missing_required_artifact_msg, FormatParam{pal_key.key(), Style::bold});
            fail_at_exit = true;
            continue;
        }
        PT_TRY_CALL_CHAIN_ERR(
            reader_->read_porymap_pal_n(*tileset, pal_key, i),
            std::unique_ptr<Tileset>,
            "Failed to read artifact '{}'.",
            FormatParam(pal_key.key(), Style::bold));
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        porymap_anim_params_key,
        key_provider_->key_for_porymap_anim_params(tileset->name()),
        std::unique_ptr<Tileset>,
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));

    PT_TRY_ASSIGN_CHAIN_ERR(
        porymap_anims,
        key_provider_->discover_porymap_anims(tileset->name()),
        std::unique_ptr<Tileset>,
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));

    for (const auto &porymap_anim_name : porymap_anims) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            frames,
            key_provider_->discover_porymap_anim_frames(tileset->name(), porymap_anim_name),
            std::unique_ptr<Tileset>,
            diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));

        std::vector<std::pair<std::string, ArtifactKey>> frames_keys{};
        for (const auto &frame_name : frames) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                frame_key,
                key_provider_->key_for_porymap_anim_frame(tileset->name(), porymap_anim_name, frame_name),
                std::unique_ptr<Tileset>,
                diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));

            if (!key_provider_->artifact_exists(frame_key)) {
                return FormattableError{missing_required_artifact_msg, FormatParam{frame_key.key(), Style::bold}};
            }
            frames_keys.emplace_back(frame_name, frame_key);
        }

        PT_TRY_CALL_CHAIN_ERR(
            reader_->read_porymap_anim(*tileset, porymap_anim_name, porymap_anim_params_key, frames_keys),
            std::unique_ptr<Tileset>,
            "Failed to read animation '{}'.",
            FormatParam(porymap_anim_name, Style::bold));
    }

    // Porytiles artifacts
    PT_TRY_ASSIGN_CHAIN_ERR(
        bottom_png_key,
        key_provider_->key_for_bottom_png(tileset->name()),
        std::unique_ptr<Tileset>,
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));
    if (key_provider_->artifact_exists(bottom_png_key)) {
        PT_TRY_CALL_CHAIN_ERR(
            reader_->read_bottom_png(*tileset, bottom_png_key),
            std::unique_ptr<Tileset>,
            "Failed to read artifact '{}'.",
            FormatParam(bottom_png_key.key(), Style::bold));
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        middle_png_key,
        key_provider_->key_for_middle_png(tileset->name()),
        std::unique_ptr<Tileset>,
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));
    if (key_provider_->artifact_exists(middle_png_key)) {
        PT_TRY_CALL_CHAIN_ERR(
            reader_->read_middle_png(*tileset, middle_png_key),
            std::unique_ptr<Tileset>,
            "Failed to read artifact '{}'.",
            FormatParam(middle_png_key.key(), Style::bold));
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        top_png_key,
        key_provider_->key_for_top_png(tileset->name()),
        std::unique_ptr<Tileset>,
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));
    if (key_provider_->artifact_exists(top_png_key)) {
        PT_TRY_CALL_CHAIN_ERR(
            reader_->read_top_png(*tileset, top_png_key),
            std::unique_ptr<Tileset>,
            "Failed to read artifact '{}'.",
            FormatParam(top_png_key.key(), Style::bold));
    }

    PT_TRY_ASSIGN_CHAIN_ERR(
        attr_csv_key,
        key_provider_->key_for_attributes_csv(tileset->name()),
        std::unique_ptr<Tileset>,
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));
    if (key_provider_->artifact_exists(attr_csv_key)) {
        PT_TRY_CALL_CHAIN_ERR(
            reader_->read_attributes_csv(*tileset, attr_csv_key),
            std::unique_ptr<Tileset>,
            "Failed to read artifact '{}'.",
            FormatParam(attr_csv_key.key(), Style::bold));
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
            std::unique_ptr<Tileset>,
            diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));
        if (key_provider_->artifact_exists(override_key)) {
            PT_TRY_CALL_CHAIN_ERR(
                reader_->read_porytiles_pal_n(*tileset, override_key, i),
                std::unique_ptr<Tileset>,
                "Failed to read artifact '{}'.",
                FormatParam(override_key.key(), Style::bold));
        }
    }

    // Load Porytiles animations using unified read method
    PT_TRY_ASSIGN_CHAIN_ERR(
        porytiles_params_key,
        key_provider_->key_for_porytiles_anim_params(tileset->name()),
        std::unique_ptr<Tileset>,
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));

    if (key_provider_->artifact_exists(porytiles_params_key)) {
        PT_TRY_ASSIGN_CHAIN_ERR(
            porytiles_anims,
            key_provider_->discover_porytiles_anims(tileset->name()),
            std::unique_ptr<Tileset>,
            diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));

        for (const auto &anim_name : porytiles_anims) {
            // Check if key frame exists (only present for automatic/hybrid frame linking)
            PT_TRY_ASSIGN_CHAIN_ERR(
                key_frame_key_candidate,
                key_provider_->key_for_porytiles_anim_frame(tileset->name(), anim_name, "key"),
                std::unique_ptr<Tileset>,
                diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));

            std::optional<ArtifactKey> key_frame_key{};
            if (key_provider_->artifact_exists(key_frame_key_candidate)) {
                key_frame_key = key_frame_key_candidate;
            }

            PT_TRY_ASSIGN_CHAIN_ERR(
                frame_names,
                key_provider_->discover_porytiles_anim_frames(tileset->name(), anim_name),
                std::unique_ptr<Tileset>,
                diag_->formatter().format("Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));

            // Build frame_keys vector for unified read
            std::vector<std::pair<std::string, ArtifactKey>> frame_keys{};
            bool anim_has_missing_frames = false;
            for (const auto &frame_name : frame_names) {
                // Skip "key" frame in frame_keys since it's handled separately via key_frame_key
                if (frame_name == "key") {
                    continue;
                }

                PT_TRY_ASSIGN_CHAIN_ERR(
                    frame_key,
                    key_provider_->key_for_porytiles_anim_frame(tileset->name(), anim_name, frame_name),
                    std::unique_ptr<Tileset>,
                    diag_->formatter().format(
                        "Failed to load tileset '{}'.", FormatParam{tileset->name(), Style::bold}));

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
            PT_TRY_CALL_CHAIN_ERR(
                reader_->read_porytiles_anim(*tileset, anim_name, porytiles_params_key, key_frame_key, frame_keys),
                std::unique_ptr<Tileset>,
                "Failed to load Porytiles animation '{}'.",
                FormatParam(anim_name, Style::bold));
        }

        // Load primary animation references from same anim.json (validated at compile time)
        PT_TRY_CALL_CHAIN_ERR(
            reader_->read_porytiles_primary_anim_references(*tileset, porytiles_params_key),
            std::unique_ptr<Tileset>,
            "Failed to load primary animation references.");
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

} // namespace porytiles
