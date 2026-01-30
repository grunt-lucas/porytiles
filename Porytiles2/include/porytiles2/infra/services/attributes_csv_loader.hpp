#pragma once

#include <filesystem>
#include <map>
#include <memory>

#include "gsl/pointers"

#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/services/behavior_map_provider.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/file_highlight_printer.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief A service that loads metatile attributes from a CSV file.
 *
 * @details
 * AttributesCsvLoader parses CSV files in the Porytiles attributes format with columns "id,behavior".
 * It validates the header, parses each row, checks for duplicates, and resolves behavior names to
 * numeric values using the provided BehaviorMapProvider. Rich error messages with file context
 * highlighting are produced via FileHighlightPrinter.
 *
 * CSV format:
 * @code
 * id,behavior
 * 0,MB_NORMAL
 * 1,MB_TALL_GRASS
 * @endcode
 */
class AttributesCsvLoader {
  public:
    /**
     * @brief Constructs an AttributesCsvLoader with required dependencies.
     *
     * @param format The text formatter for styled output
     * @param behavior_map The behavior map provider for resolving behavior names to values
     */
    AttributesCsvLoader(
        gsl::not_null<const TextFormatter *> format, gsl::not_null<const BehaviorMapProvider *> behavior_map)
        : format_{format}, behavior_map_{behavior_map}, file_printer_{std::make_unique<FileHighlightPrinter>(format)}
    {
    }

    /**
     * @brief Loads metatile attributes from a CSV file.
     *
     * @details
     * Parses the CSV file, validates format, resolves behaviors, and returns a map of
     * metatile ID to MetatileAttribute. All attributes are created with LayerType::normal.
     *
     * @param path The path to the attributes CSV file
     * @pre File must exist and be readable
     * @return Map of metatile IDs to their attributes, or an error with file context
     */
    [[nodiscard]] ChainableResult<std::map<std::size_t, MetatileAttribute>>
    load(const std::filesystem::path &path) const;

  private:
    const TextFormatter *format_;
    const BehaviorMapProvider *behavior_map_;
    const std::unique_ptr<FileHighlightPrinter> file_printer_;
};

} // namespace porytiles2
