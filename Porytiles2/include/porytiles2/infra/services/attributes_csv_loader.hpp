#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <optional>

#include "gsl/pointers"

#include "porytiles2/domain/models/base_game.hpp"
#include "porytiles2/domain/models/metatile_attribute.hpp"
#include "porytiles2/domain/services/behavior_map_provider.hpp"
#include "porytiles2/domain/services/encounter_type_map_provider.hpp"
#include "porytiles2/domain/services/terrain_type_map_provider.hpp"
#include "porytiles2/utilities/result/chainable_result.hpp"
#include "porytiles2/utilities/text/file_highlight_printer.hpp"
#include "porytiles2/utilities/text/text_formatter.hpp"

namespace porytiles2 {

/**
 * @brief A service that loads metatile attributes from a CSV file.
 *
 * @details
 * AttributesCsvLoader parses CSV files in two supported Porytiles attributes formats:
 *
 * Emerald format (2 columns):
 * @code
 * id,behavior
 * 0,MB_NORMAL
 * 1,MB_TALL_GRASS
 * @endcode
 *
 * FireRed format (4 columns):
 * @code
 * id,behavior,terrainType,encounterType
 * 0,MB_NORMAL,TILE_TERRAIN_NORMAL,TILE_ENCOUNTER_NONE
 * 1,MB_TALL_GRASS,TILE_TERRAIN_GRASS,TILE_ENCOUNTER_LAND
 * @endcode
 *
 * The format is auto-detected from the CSV header row. If a @c BaseGame is provided, the loader
 * validates that the detected format matches the expected base game. Rich error messages with file
 * context highlighting are produced via FileHighlightPrinter.
 */
class AttributesCsvLoader {
  public:
    /**
     * @brief Constructs an AttributesCsvLoader with required dependencies.
     *
     * @details
     * This constructor provides backward compatibility with the original 2-column loader. The optional
     * parameters enable FireRed 4-column CSV support when provided.
     *
     * @param format The text formatter for styled output
     * @param behavior_map The behavior map provider for resolving behavior names to values
     * @param base_game Optional base game for format validation against the detected CSV format
     * @param terrain_map Optional terrain type provider (required for FireRed CSV format)
     * @param encounter_map Optional encounter type provider (required for FireRed CSV format)
     */
    AttributesCsvLoader(
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const BehaviorMapProvider *> behavior_map,
        std::optional<BaseGame> base_game = std::nullopt,
        const TerrainTypeMapProvider *terrain_map = nullptr,
        const EncounterTypeMapProvider *encounter_map = nullptr)
        : format_{format}, behavior_map_{behavior_map}, base_game_{base_game}, terrain_map_{terrain_map},
          encounter_map_{encounter_map}, file_printer_{std::make_unique<FileHighlightPrinter>(format)}
    {
    }

    /**
     * @brief Loads metatile attributes from a CSV file.
     *
     * @details
     * Parses the CSV file, auto-detects format from the header row, validates format against
     * the base game (if provided), resolves behaviors (and terrain/encounter types for FireRed),
     * and returns a map of metatile ID to MetatileAttribute. All attributes are created with
     * LayerType::normal.
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
    std::optional<BaseGame> base_game_;
    const TerrainTypeMapProvider *terrain_map_;
    const EncounterTypeMapProvider *encounter_map_;
    const std::unique_ptr<FileHighlightPrinter> file_printer_;
};

} // namespace porytiles2
