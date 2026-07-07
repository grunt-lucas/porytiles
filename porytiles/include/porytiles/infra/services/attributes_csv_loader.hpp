#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "gsl/pointers"

#include "porytiles/domain/models/base_game.hpp"
#include "porytiles/domain/models/metatile_attribute.hpp"
#include "porytiles/domain/services/enum_map_provider.hpp"
#include "porytiles/infra/config/infra_config.hpp"
#include "porytiles/utilities/result/chainable_result.hpp"
#include "porytiles/utilities/text/file_highlight_printer.hpp"
#include "porytiles/utilities/text/text_formatter.hpp"
#include "porytiles/xcut/diagnostics/user_diagnostics.hpp"

namespace porytiles {

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
     * @param config The config used to resolve the write_layer_type_column knob at tileset scope
     * @param diag The diagnostics sink for the "column ignored" warning when the knob is off
     * @param base_game Optional base game for format validation against the detected CSV format
     * @param terrain_map Optional terrain type provider (required for FireRed CSV format)
     * @param encounter_map Optional encounter type provider (required for FireRed CSV format)
     */
    AttributesCsvLoader(
        gsl::not_null<const TextFormatter *> format,
        gsl::not_null<const EnumMapProvider *> behavior_map,
        gsl::not_null<const InfraConfig *> config,
        gsl::not_null<const UserDiagnostics *> diag,
        std::optional<BaseGame> base_game = std::nullopt,
        const EnumMapProvider *terrain_map = nullptr,
        const EnumMapProvider *encounter_map = nullptr)
        : format_{format}, behavior_map_{behavior_map}, config_{config}, diag_{diag}, base_game_{base_game},
          terrain_map_{terrain_map}, encounter_map_{encounter_map},
          file_printer_{std::make_unique<FileHighlightPrinter>(format)}
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
     * When a @c layerType column is present, its handling depends on the write_layer_type_column knob resolved at the
     * scope of @p tileset_name: with the knob on, a filled cell pins the attribute's layer type and a blank cell leaves
     * it inferred; with the knob off, the column is ignored and a single warning is emitted for the file.
     *
     * @param path The path to the attributes CSV file
     * @param tileset_name The tileset whose config scope resolves the write_layer_type_column knob. When compiling a
     * secondary this is the primary's name for the primary's CSV, so the knob resolves under the file's owning tileset.
     * @pre File must exist and be readable
     * @return Map of metatile IDs to their attributes, or an error with file context
     */
    [[nodiscard]] ChainableResult<std::map<std::size_t, MetatileAttribute>>
    load(const std::filesystem::path &path, const std::string &tileset_name) const;

  private:
    const TextFormatter *format_;
    const EnumMapProvider *behavior_map_;
    const InfraConfig *config_;
    const UserDiagnostics *diag_;
    std::optional<BaseGame> base_game_;
    const EnumMapProvider *terrain_map_;
    const EnumMapProvider *encounter_map_;
    const std::unique_ptr<FileHighlightPrinter> file_printer_;
};

} // namespace porytiles
