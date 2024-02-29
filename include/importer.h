#ifndef PORYTILES_IMPORTER_H
#define PORYTILES_IMPORTER_H

#include <filesystem>
#include <png.hpp>
#include <string>
#include <unordered_map>

#include "porytiles_context.h"
#include "types.h"

/**
 * Utility functions for building core Porytiles types from sanitized input types and data structures. The importer
 * functions don't handle any file checking or input collation. They expect to receive data as ready-to-use ifstreams,
 * png::images, and other data structures. The driver is responsible for preparing these types and data structures from
 * the raw input files.
 */

namespace porytiles {

/**
 * Build a DecompiledTileset from a single source PNG. This tileset is considered "raw", that is, it has no layering.
 * The importer will simply scan the PNG tiles left-to-right, top-to-bottom and put them into the DecompiledTileset.
 */
DecompiledTileset importTilesFromPng(PorytilesContext &ctx, const png::image<png::rgba_pixel> &png);

/**
 * TODO : fill in doc comments
 */
DecompiledTileset importLayeredTilesFromPngs(PorytilesContext &ctx,
                                             const std::unordered_map<std::size_t, Attributes> &attributesMap,
                                             const png::image<png::rgba_pixel> &bottom,
                                             const png::image<png::rgba_pixel> &middle,
                                             const png::image<png::rgba_pixel> &top);

void importAnimTiles(PorytilesContext &ctx, const std::vector<std::vector<AnimationPng<png::rgba_pixel>>> &rawAnims,
                     DecompiledTileset &tiles);

/**
 * TODO : fill in doc comments
 */
std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>>
importMetatileBehaviorHeader(PorytilesContext &ctx, std::ifstream &behaviorFile);

/**
 * TODO : fill in doc comments
 */
std::unordered_map<std::size_t, Attributes>
importAttributesFromCsv(PorytilesContext &ctx, const std::unordered_map<std::string, std::uint8_t> &behaviorMap,
                        const std::string &filePath);

/**
 * TODO : fill in doc comments
 */
void importPrimaryAssignmentCache(PorytilesContext &ctx, std::ifstream &config);

/**
 * TODO : fill in doc comments
 */
void importSecondaryAssignmentConfigParameters(PorytilesContext &ctx, std::ifstream &config);

/**
 * TODO : fill in doc comments
 */
std::pair<CompiledTileset, std::unordered_map<std::size_t, Attributes>>
importCompiledTileset(PorytilesContext &ctx, std::ifstream &metatiles, std::ifstream &attributes,
                      const std::unordered_map<std::uint8_t, std::string> &behaviorReverseMap,
                      const png::image<png::index_pixel> &tilesheetPng,
                      const std::vector<std::unique_ptr<std::ifstream>> &paletteFiles,
                      const std::vector<std::vector<AnimationPng<png::index_pixel>>> &compiledAnims);

/**
 * TODO : fill in doc comments
 */
RGBATile importPalettePrimer(PorytilesContext &ctx, std::ifstream &paletteFile);

} // namespace porytiles

#endif // PORYTILES_IMPORTER_H
