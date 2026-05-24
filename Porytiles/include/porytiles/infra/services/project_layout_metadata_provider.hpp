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

/**
 * @brief Provides a pokeemerald project filesystem-based implementation for LayoutMetadataProvider.
 *
 * @details
 * This class parses layout definitions from layouts.json to extract metadata such a layout's width/height, primary
 * tileset, border filepath, etc.
 *
 * Layout metadata is lazy-loaded and cached for efficiency.
 */
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

    /**
     * @brief Resolves border file path for a layout by parsing layouts.json.
     *
     * @param layout_name_or_id The name or ID of the layout to check (e.g., "PetalburgCity_Layout" or
     * "LAYOUT_PETALBURG_CITY")
     * @pre layout_name_or_id must refer to an existing layout on disk
     * @return The resolved border filepath for the layout
     */
    [[nodiscard]] ChainableResult<std::filesystem::path> border_filepath(const std::string &layout_name_or_id) const;

    /**
     * @brief Resolves blockdata file path for a layout by parsing layouts.json.
     *
     * @param layout_name_or_id The name or ID of the layout to check (e.g., "PetalburgCity_Layout" or
     * "LAYOUT_PETALBURG_CITY")
     * @pre layout_name_or_id must refer to an existing layout on disk
     * @return The resolved blockdata filepath for the layout
     */
    [[nodiscard]] ChainableResult<std::filesystem::path> blockdata_filepath(const std::string &layout_name_or_id) const;

    /**
     * @brief Invalidates the lazy-loaded layout cache, forcing a re-parse on next access.
     *
     * @details
     * This is needed when the underlying layouts.json file has been modified on disk since the cache was populated.
     * For example, after creating a new layout entry, the cache must be invalidated so subsequent lookups see the
     * newly written layout.
     */
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
