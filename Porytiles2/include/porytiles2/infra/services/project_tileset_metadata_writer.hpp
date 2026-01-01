#pragma once

#include <filesystem>
#include <string>

#include "gsl/pointers"

#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief Provides surgical update capability for tileset metadata in headers.h files.
 *
 * @details
 * This class enables targeted modifications to specific fields within tileset struct declarations in pokeemerald
 * headers.h files. Unlike a full rewrite approach, it performs surgical line-level edits to preserve formatting,
 * comments, and other content in the file.
 *
 * Currently supports updating the .callback field for animation callback registration.
 */
class ProjectTilesetMetadataWriter {
  public:
    ProjectTilesetMetadataWriter(std::filesystem::path project_root, gsl::not_null<const TextFormatter *> format);

    /**
     * @brief Updates the .callback field for a specific tileset.
     *
     * @details
     * Locates the tileset struct by name in headers.h and surgically replaces the .callback field value. The new value
     * is written exactly as provided - callers should pass either "NULL" or "InitTilesetAnim_<name>" format.
     *
     * @param tileset_name The tileset name (e.g., "gTileset_General") - must match exactly
     * @param new_callback_value The exact value to write (e.g., "InitTilesetAnim_General" or "NULL")
     * @pre tileset_name must correspond to an existing tileset in headers.h
     * @pre new_callback_value must be a valid C identifier or "NULL"
     * @return Success or error result with details
     */
    [[nodiscard]] ChainableResult<void>
    update_callback(const std::string &tileset_name, const std::string &new_callback_value) const;

  private:
    std::filesystem::path project_root_;
    const TextFormatter *format_;
};

} // namespace porytiles2
