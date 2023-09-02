#ifndef PORYTILES_IMPORTER_H
#define PORYTILES_IMPORTER_H

#include <filesystem>
#include <png.hpp>
#include <string>
#include <unordered_map>

#include "ptcontext.h"
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
DecompiledTileset importTilesFromPng(PtContext &ctx, const png::image<png::rgba_pixel> &png);

DecompiledTileset importLayeredTilesFromPngs(PtContext &ctx,
                                             const std::unordered_map<std::size_t, Attributes> &attributesMap,
                                             const png::image<png::rgba_pixel> &bottom,
                                             const png::image<png::rgba_pixel> &middle,
                                             const png::image<png::rgba_pixel> &top);

void importAnimTiles(PtContext &ctx, const std::vector<std::vector<AnimationPng<png::rgba_pixel>>> &rawAnims,
                     DecompiledTileset &tiles);

std::pair<std::unordered_map<std::string, std::uint8_t>, std::unordered_map<std::uint8_t, std::string>>
importMetatileBehaviorMaps(PtContext &ctx, std::ifstream &behaviorFile);

std::unordered_map<std::size_t, Attributes>
importAttributesFromCsv(PtContext &ctx, const std::unordered_map<std::string, std::uint8_t> &behaviorMap,
                        const std::string &filePath);

CompiledTileset importCompiledTileset(PtContext &ctx, const std::filesystem::path &tilesetPath);

} // namespace porytiles

#endif // PORYTILES_IMPORTER_H
