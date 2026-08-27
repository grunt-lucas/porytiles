#include "porytiles/app/use_cases/compile_secondary_tileset.hpp"

#include <memory>
#include <string>

#include "porytiles/app/use_cases/secondary_tileset_helpers.hpp"
#include "porytiles/domain/algorithms/diagnostic_stencils.hpp"
#include "porytiles/domain/models/tileset.hpp"
#include "porytiles/domain/services/tileset_compiler.hpp"
#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/xcut/config/unwrap_config.hpp"

namespace porytiles {

ChainableResult<void> CompileSecondaryTileset::compile(const std::string &tileset_name) const
{
    // 1. Precondition: tileset must exist. Check if it is Porytiles-managed.
    assert_or_panic(metadata_provider_->exists(tileset_name), "precondition violated: tileset must exist");
    if (!tileset_manager_->is_porytiles_managed(tileset_name)) {
        return FormattableError{
            "Tileset '{}' exists but is not Porytiles-managed.", FormatParam{tileset_name, Style::bold}};
    }

    // 2. Read primary pairing configuration.
    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, primary_pairing_mode, tileset_name, void);
    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, primary_pairing_partners, tileset_name, void);

    // 3. Resolve the partner primary tileset.
    PT_TRY_ASSIGN_CHAIN_ERR(
        paired_primary,
        resolve_partner_primary(
            tileset_name,
            primary_pairing_mode,
            primary_pairing_partners,
            tileset_repo_,
            metadata_provider_,
            layout_metadata_provider_,
            tileset_manager_,
            diag_),
        void,
        "Failed to resolve partner primary for secondary '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 4. Load the secondary tileset.
    PT_TRY_ASSIGN_CHAIN_ERR(
        tileset,
        tileset_repo_->load(tileset_name),
        void,
        diag_->formatter().format("Failed to load tileset '{}'.", FormatParam(tileset_name, Style::bold)));

    // 4b. Cross-tileset extrinsic transparency mismatch warning.
    {
        PT_UNWRAP_TILESET_CONFIG_PTR_AS(secondary_et, domain_config_, extrinsic_transparency, tileset_name, void);
        PT_UNWRAP_TILESET_CONFIG_PTR_AS(
            primary_et, domain_config_, extrinsic_transparency, paired_primary->name(), void);
        if (secondary_et.value() != primary_et.value()) {
            std::vector<std::string> warn_msg{};
            warn_msg.emplace_back(diag_->formatter().format(
                "Secondary tileset '{}' and its paired primary '{}' have different configured "
                "'extrinsic_transparency' values ('{}' vs '{}').",
                FormatParam{tileset_name, Style::bold},
                FormatParam{paired_primary->name(), Style::bold},
                FormatParam{secondary_et.value().to_jasc_str(), Style::bold},
                FormatParam{primary_et.value().to_jasc_str(), Style::bold}));
            warn_msg.emplace_back(
                "Cross-tileset keyframe matching will use each side's configured value independently.");
            warn_msg.emplace_back("");
            warn_msg.emplace_back(
                "This warning is based on the current domain config, not on the ET each tileset was actually "
                "compiled under.");
            warn_msg.emplace_back(
                "If a tileset was compiled with a different ET and has not been recompiled since, cross-tileset "
                "matching may produce unexpected results.");
            warn_msg.emplace_back("Recompile both tilesets if in doubt.");
            warn_msg.emplace_back("");
            warn_msg.emplace_back(diagnostic_separator);
            warn_msg.emplace_back("");
            warn_msg.append_range(format_config_note(diag_->formatter(), secondary_et));
            warn_msg.emplace_back("");
            warn_msg.emplace_back(diagnostic_separator);
            warn_msg.emplace_back("");
            warn_msg.append_range(format_config_note(diag_->formatter(), primary_et));

            diag_->warning("cross-tileset-extrinsic-transparency-mismatch", warn_msg);
        }
    }

    // 5. Checksum verification.
    PT_UNWRAP_TILESET_CONFIG_PTR(app_config_, verify_checksums, tileset_name, void);
    if (!tileset_repo_->checksum_provider().cached_checksums_exist(tileset_name) && verify_checksums) {
        std::vector<std::string> err_msg{};
        err_msg.emplace_back(diag_->formatter().format(
            "No cached checksums found for tileset '{}'.", FormatParam{tileset_name, Style::bold}));
        err_msg.emplace_back(diag_->formatter().format(
            "Expected to find file '{}'.",
            FormatParam{"porytiles/tilesets/" + tileset_name + "/tileset.cache.json", Style::bold}));
        err_msg.emplace_back("Checksum verification requested via configuration.");
        err_msg.append_range(format_config_note_with_separator(diag_->formatter(), verify_checksums));

        return ChainableResult<void>{FormattableError{err_msg}};
    }

    if (tileset_repo_->checksum_provider().cached_checksums_exist(tileset_name) && verify_checksums.value()) {
        // Check for unimported Porymap asset changes.
        if (!tileset->porymap_component().is_empty()) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                porymap_keys,
                tileset_repo_->key_provider().get_porymap_artifact_keys(tileset_name),
                void,
                "Failed to get Porymap artifact keys.");
            const auto mismatched_keys =
                tileset_repo_->checksum_provider().find_unsynced_tileset_artifacts(tileset_name, porymap_keys);
            if (!mismatched_keys.empty()) {
                std::vector<std::string> err_msg{};
                err_msg.emplace_back("Changes present in Porymap assets:");
                for (const auto &key : mismatched_keys) {
                    err_msg.emplace_back(diag_->formatter().format("  {}", FormatParam{key.key(), Style::bold}));
                }
                err_msg.emplace_back("");
                err_msg.emplace_back("Compiling now would clobber your Porymap asset changes.");
                err_msg.emplace_back("To resolve:");
                err_msg.emplace_back(diag_->formatter().format(
                    "  - Run '{} {}' to synchronize assets.",
                    FormatParam{"decompile-tileset", Style::bold},
                    FormatParam{tileset_name, Style::bold}));
                err_msg.emplace_back(diag_->formatter().format(
                    "  - {} disable checksum verification to allow the clobber.", FormatParam{"OR", Style::bold}));
                err_msg.emplace_back(diag_->formatter().format(
                    "  - {} delete '{}' cache file.",
                    FormatParam{"OR", Style::bold},
                    FormatParam{"porytiles/tilesets/" + tileset_name + "/tileset.cache.json", Style::bold}));
                err_msg.append_range(format_config_note_with_separator(diag_->formatter(), verify_checksums));
                return ChainableResult<void>{FormattableError{err_msg}};
            }
        }

        // Primary-side staleness check. Content-aware, unlike the name-only check in
        // pipeline_helper_register_animations. If the paired primary's sources have drifted from its cached compile
        // form, refuse to proceed: cross-tileset matching would operate against a stale compiled primary and silently
        // produce wrong output. Only runs when the paired primary itself has cached checksums; a primary with no cache
        // cannot be content-verified here (the user may have compiled it externally).
        if (tileset_repo_->checksum_provider().cached_checksums_exist(paired_primary->name())) {
            PT_TRY_ASSIGN_CHAIN_ERR(
                primary_porytiles_keys,
                tileset_repo_->key_provider().get_porytiles_artifact_keys(paired_primary->name()),
                void,
                "Failed to get Porytiles artifact keys for paired primary '{}'.",
                FormatParam(paired_primary->name(), Style::bold));
            const auto primary_mismatched_keys = tileset_repo_->checksum_provider().find_unsynced_tileset_artifacts(
                paired_primary->name(), primary_porytiles_keys);
            if (!primary_mismatched_keys.empty()) {
                std::vector<std::string> err_msg{};
                err_msg.emplace_back(diag_->formatter().format(
                    "Paired primary tileset '{}' has source changes that have not been compiled.",
                    FormatParam{paired_primary->name(), Style::bold}));
                err_msg.emplace_back("Changes present in paired primary Porytiles sources:");
                for (const auto &key : primary_mismatched_keys) {
                    err_msg.emplace_back(diag_->formatter().format("  {}", FormatParam{key.key(), Style::bold}));
                }
                err_msg.emplace_back("");
                err_msg.emplace_back("Compiling the secondary now would match tiles against a stale compiled primary.");
                err_msg.emplace_back("To resolve:");
                err_msg.emplace_back(diag_->formatter().format(
                    "  - Run '{} {}' to bring the primary's compiled form up to date.",
                    FormatParam{"compile-tileset", Style::bold},
                    FormatParam{paired_primary->name(), Style::bold}));
                err_msg.append_range(format_config_note_with_separator(diag_->formatter(), verify_checksums));
                return ChainableResult<void>{FormattableError{err_msg}};
            }
        }

        // Check if Porytiles source assets are unchanged (nothing to do).
        PT_TRY_ASSIGN_CHAIN_ERR(
            porytiles_keys,
            tileset_repo_->key_provider().get_porytiles_artifact_keys(tileset_name),
            void,
            "Failed to get Porytiles artifact keys.");
        if (tileset_repo_->checksum_provider().all_checksums_tileset_match(tileset_name, porytiles_keys)) {
            diag_->warning(
                "nothing-to-do",
                "Skipping compilation for '{}', no changes found.",
                FormatParam{tileset_name, Style::bold});
            return {};
        }
    }

    // 6. Compile the secondary tileset.
    PT_TRY_ASSIGN_CHAIN_ERR(
        new_tileset,
        compiler_->compile(*tileset, /*is_secondary=*/true, paired_primary.get()),
        void,
        "Compilation job failed for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 7. Persist the compiled tileset (which also caches the checksums).
    PT_TRY_CALL_CHAIN_ERR(
        tileset_repo_->save(*new_tileset),
        void,
        "Tileset save job failed for '{}'.",
        FormatParam(tileset_name, Style::bold));

    // 8. Handle animation code wiring.
    if (!new_tileset->porymap_component().anims().empty()) {
        PT_TRY_CALL_CHAIN_ERR(
            tileset_manager_->wire_anim_code(tileset_name, /*is_secondary=*/true),
            void,
            "Failed to wire animation code for '{}'.",
            FormatParam(tileset_name, Style::bold));
    }
    else {
        PT_TRY_CALL_CHAIN_ERR(
            tileset_manager_->remove_wired_anim_code(tileset_name, /*is_secondary=*/true),
            void,
            "Failed to remove wired animation code for '{}'.",
            FormatParam(tileset_name, Style::bold));
    }

    return {};
}

} // namespace porytiles
