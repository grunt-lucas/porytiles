#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "gsl/pointers"

#include "porytiles/domain/services/layout_metadata_provider.hpp"
#include "porytiles/infra/models/project_layout_metadata.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

/// @brief How the layouts that reference a given tileset classify under their layout_version fields.
///
/// @details
/// Computed by ProjectLayoutMetadataProvider::layout_version_usage. Drives FRLG alternate mask selection in automatic
/// mode: frlg_only picks the alternate masks, emerald_only and unreferenced pick the primary masks, and mixed is a hard
/// error because a single tileset cannot carry two different binary schemas.
enum class TilesetLayoutVersionUsage {
    unreferenced, ///< No layout references the tileset (or layouts.json is absent).
    emerald_only, ///< Every referencing layout is emerald (or omits layout_version).
    frlg_only,    ///< Every referencing layout is frlg.
    mixed,        ///< Referencing layouts disagree: some emerald, some frlg.
};

[[nodiscard]] inline std::string to_string(TilesetLayoutVersionUsage usage)
{
    switch (usage) {
    case TilesetLayoutVersionUsage::unreferenced:
        return "unreferenced";
    case TilesetLayoutVersionUsage::emerald_only:
        return "emerald_only";
    case TilesetLayoutVersionUsage::frlg_only:
        return "frlg_only";
    case TilesetLayoutVersionUsage::mixed:
        return "mixed";
    }
    return "unreferenced";
}

/// @brief Provides a pokeemerald project filesystem-based implementation for LayoutMetadataProvider.
///
/// @details
/// This class parses layout definitions from layouts.json to extract metadata such a layout's width/height, primary
/// tileset, border filepath, etc.
///
/// Layout metadata is lazy-loaded and cached for efficiency.
class ProjectLayoutMetadataProvider : public LayoutMetadataProvider {
  public:
    ProjectLayoutMetadataProvider(
        std::filesystem::path project_root,
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const UserDiagnostics *> diag)
        : project_root_{std::move(project_root)}, format_{format}, diag_{diag}
    {
    }

    [[nodiscard]] bool exists(const std::string &layout_name_or_id) const override;

    [[nodiscard]] ChainableResult<std::size_t> width(const std::string &layout_name_or_id) const override;

    [[nodiscard]] ChainableResult<std::size_t> height(const std::string &layout_name_or_id) const override;

    [[nodiscard]] ChainableResult<std::string> primary_tileset(const std::string &layout_name_or_id) const override;

    [[nodiscard]] ChainableResult<std::string> secondary_tileset(const std::string &layout_name_or_id) const override;

    [[nodiscard]] ChainableResult<std::set<std::string>> layout_names() const override;

    [[nodiscard]] ChainableResult<std::set<std::string>> layout_ids() const override;

    [[nodiscard]] ChainableResult<std::string> layouts_table_label() const;

    /// @brief Classifies how the layouts referencing a tileset set their layout_version.
    ///
    /// @details
    /// Scans every layout whose primary_tileset or secondary_tileset equals @p tileset_label and buckets by
    /// layout_version: absent or exactly "emerald" counts as emerald, exactly "frlg" counts as frlg. A missing
    /// layouts.json yields @c unreferenced (checked before parsing). A malformed layouts.json propagates the existing
    /// parse error. Any layout_version value other than exactly "emerald" or "frlg" on a referencing layout is a hard
    /// error, because this string controls the binary attribute schema width and a typo like "firered" must not
    /// silently mean emerald. Comparison is exact and case-sensitive, matching mapjson.
    ///
    /// @param tileset_label The tileset label as referenced in layouts.json (e.g., "gTileset_General"). Commands pass
    /// the gTileset_* label as both the tileset name and config scope, which is exactly what layouts.json references,
    /// so no name mapping is needed.
    /// @return The usage classification, or an error for malformed JSON / an invalid layout_version value.
    [[nodiscard]] ChainableResult<TilesetLayoutVersionUsage>
    layout_version_usage(const std::string &tileset_label) const;

    /// @brief Resolves border file path for a layout by parsing layouts.json.
    ///
    /// @param layout_name_or_id The name or ID of the layout to check (e.g., "PetalburgCity_Layout" or
    /// "LAYOUT_PETALBURG_CITY")
    /// @pre layout_name_or_id must refer to an existing layout on disk
    /// @return The resolved border filepath for the layout
    [[nodiscard]] ChainableResult<std::filesystem::path> border_filepath(const std::string &layout_name_or_id) const;

    /// @brief Resolves blockdata file path for a layout by parsing layouts.json.
    ///
    /// @param layout_name_or_id The name or ID of the layout to check (e.g., "PetalburgCity_Layout" or
    /// "LAYOUT_PETALBURG_CITY")
    /// @pre layout_name_or_id must refer to an existing layout on disk
    /// @return The resolved blockdata filepath for the layout
    [[nodiscard]] ChainableResult<std::filesystem::path> blockdata_filepath(const std::string &layout_name_or_id) const;

    /// @brief Invalidates the lazy-loaded layout cache, forcing a re-parse on next access.
    ///
    /// @details
    /// This is needed when the underlying layouts.json file has been modified on disk since the cache was populated.
    /// For example, after creating a new layout entry, the cache must be invalidated so subsequent lookups see the
    /// newly written layout.
    void invalidate_metadata_cache() const;

  private:
    std::filesystem::path project_root_;
    const TextFormatter *format_;
    const UserDiagnostics *diag_;

    // Lazy-loaded cache for parsed layout data (mutable for const methods)
    mutable bool layouts_parsed_{false};
    mutable std::string layouts_table_label_;
    mutable std::vector<ProjectLayoutMetadata> layout_entries_;
    mutable std::map<std::string, std::size_t> layout_index_;
};

} // namespace porytiles
