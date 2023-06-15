#include "tileset.h"

#include "rgb_color.h"
#include "cli_parser.h"
#include "tsoutput.h"
#include "tile.h"
#include "palette.h"
#include "tsexception.h"

#include <algorithm>
#include <sstream>
#include <filesystem>
#include <png.hpp>

namespace porytiles {
namespace {
bool areAllTileColorsUnseen(std::unordered_set<RgbColor> uniqueTileColors,
                            std::vector<std::unordered_set<RgbColor>> unseenTileColors) {
    return std::all_of(unseenTileColors.begin(), unseenTileColors.end(),
                       [&uniqueTileColors](std::unordered_set<RgbColor>& colors) {
                           return uniqueTileColors.size() == colors.size();
                       });
}

int getClosestPaletteWithRoom(const RgbTiledPng& masterTiles, int tileIndex, std::vector<Palette>& palettes,
                              std::vector<std::unordered_set<RgbColor>> unseenTileColors) {
    int indexOfClosestPalette = -1;
    // Palettes can only have 15 colors, so 1000 will work fine as a starting value
    size_t missingColorCount = 1000;
    for (size_t i = 0; i < unseenTileColors.size(); i++) {
        if (unseenTileColors[i].size() < missingColorCount &&
            unseenTileColors[i].size() <= palettes[i].remainingColors()) {
            /*
             * TODO : do we also want to condition this on palette size? Right now: if two (or more) palettes are
             * equally close, and each indeed has room, we just return the first palette that satisfies both conditions.
             * Do we also want to make sure we return the smaller of the two (or more)?
             */
            missingColorCount = unseenTileColors[i].size();
            indexOfClosestPalette = static_cast<int>(i);
        }
    }
    if (indexOfClosestPalette == -1) {
        // This happens when no palette has room
        throw TsException{masterTiles.tileDebugString(tileIndex) + ": not enough palettes to allocate this tile"};
    }
    return indexOfClosestPalette;
}

void assignTileToPalette(const RgbTiledPng& masterTiles, int tileIndex, std::vector<Palette>& palettes) {
    const RgbTile& tile = masterTiles.tileAt(tileIndex);

    /*
     * If this is a total transparent tile, skip it.
     */
    if (tile.isUniformly(gOptTransparentColor)) {
        verboseLog("skipping transparent " + masterTiles.tileDebugString(tileIndex));
        return;
    }

    /*
     * If this is a primer control tile, skip it.
     */
    if (tile.isUniformly(gOptPrimerColor)) {
        verboseLog("skipping primer control " + masterTiles.tileDebugString(tileIndex));
        return;
    }

    /*
     * If this is a sibling control tile, skip it.
     */
    if (tile.isUniformly(gOptSiblingColor)) {
        verboseLog("skipping sibling control " + masterTiles.tileDebugString(tileIndex));
        return;
    }

    /*
     * Add each RgbColor in the tile to an unordered_set, `uniqueTileColors`
     */
    std::unordered_set<RgbColor> uniqueTileColors = tile.uniquePixels(gOptTransparentColor);
    std::string logString = masterTiles.tileDebugString(tileIndex) + ": " +
                            std::to_string(uniqueTileColors.size()) + " unique color(s): [";
    for (const auto& color: uniqueTileColors) {
        logString += color.prettyString() + "; ";
    }
    logString += "]";
    verboseLog(logString);
    logString.clear();

    /*
     * Compute `unseenTileColors[i] = uniqueTileColors - palette[i]` for each palette, where `-` is set difference.
     */
    std::vector<std::unordered_set<RgbColor>> unseenTileColors;
    unseenTileColors.reserve(palettes.size());
    for (size_t j = 0; j < palettes.size(); j++) {
        const Palette& palette = palettes.at(j);
        unseenTileColors.push_back(tile.pixelsNotInPalette(palette));
        logString = masterTiles.tileDebugString(tileIndex) + ": " +
                    std::to_string(unseenTileColors[j].size()) + " color(s) not in palette " +
                    std::to_string(j) + "(s=" + std::to_string(palettes[j].size()) + "): [";
        for (const auto& color: unseenTileColors[j]) {
            logString += color.prettyString() + "; ";
        }
        logString += "]";
        verboseLog(logString);
        logString.clear();
    }

    /*
     * If `size(unseenTileColors[i]) == 0`, that means `palette[i]` fully covers `uniqueTileColors`, and so we can
     * return, we have found a palette that will work for this tile.
     */
    for (size_t j = 0; j < unseenTileColors.size(); j++) {
        const std::unordered_set<RgbColor>& diff = unseenTileColors[j];
        if (diff.empty()) {
            logString = masterTiles.tileDebugString(tileIndex) + ": matched palette " + std::to_string(j);
            verboseLog(logString);
            logString.clear();
            return;
        }
    }

    /*
     * If `size(unseenTileColors[i]) == size(uniqueTileColors)`, that means this tile doesn't share any colors with
     * `palette[i]`. If this is true for every palette, then we have a tile with only new colors. In this case, let's
     * add the new colors to the palette with the fewest number of colors so far.
     */
    bool tileColorsAllUnseen = areAllTileColorsUnseen(uniqueTileColors, unseenTileColors);
    if (tileColorsAllUnseen) {
        int palWithFewestColors = Palette::paletteWithFewestColors(palettes);
        if (uniqueTileColors.size() > palettes[palWithFewestColors].remainingColors()) {
            throw TsException{masterTiles.tileDebugString(tileIndex) + ": not enough palettes to allocate this tile"};
        }
        for (const auto& color: uniqueTileColors) {
            palettes[palWithFewestColors].addColorAtEnd(color);
        }
        logString = masterTiles.tileDebugString(tileIndex) + ": all colors were unseen, added to palette " +
                    std::to_string(palWithFewestColors) + ", " +
                    std::to_string(palettes[palWithFewestColors].remainingColors()) + " colors remaining";
        verboseLog(logString);
        logString.clear();
        return;
    }

    /*
     * If `size(unseenTileColors[i]) > 0`, `uniqueTileColors` shares some, but not all, colors with `palette[i]`. Save
     * off how many. Do this for each palette, we'll want to use the palette that is the closest match. If the closest
     * match palette doesn't have room for the new colors, try the next closest match. If no palette has room, fail the
     * whole program
     */
    int closestPalWithRoom = getClosestPaletteWithRoom(masterTiles, tileIndex, palettes, unseenTileColors);
    for (const auto& color: unseenTileColors[closestPalWithRoom]) {
        palettes[closestPalWithRoom].addColorAtEnd(color);
    }
    logString = masterTiles.tileDebugString(tileIndex) + ": contained some new colors, added to palette " +
                std::to_string(closestPalWithRoom) + ", " +
                std::to_string(palettes[closestPalWithRoom].remainingColors()) + " colors remaining";
    verboseLog(logString);
    logString.clear();
}

void
indexTile(const RgbTiledPng& masterTiles, int tileIdx, std::vector<Palette>& palettes,
          std::vector<IndexedTile>& tiles,
          std::unordered_set<IndexedTile>& tilesIndex) {
    std::string logString;
    const RgbTile& tile = masterTiles.tileAt(tileIdx);
    IndexedTile indexedTile;

    /*
     * If this is a total transparent tile, skip it.
     */
    if (tile.isUniformly(gOptTransparentColor)) {
        verboseLog("skipping transparent " + masterTiles.tileDebugString(tileIdx));
        return;
    }

    /*
     * If this is a sibling control tile, skip it.
     */
    if (tile.isUniformly(gOptSiblingColor)) {
        verboseLog("skipping sibling control " + masterTiles.tileDebugString(tileIdx));
        return;
    }

    /*
     * Find the first matching palette for this tile. The previous palette construction step guarantees that one exists.
     * If for some reason one cannot be found, throw a fatal error and exit.
     */
    size_t matchingPaletteIndex = 0;
    for (size_t j = 0; j < palettes.size(); j++) {
        const Palette& palette = palettes.at(j);
        if (tile.pixelsNotInPalette(palette).empty()) {
            matchingPaletteIndex = j;
            logString = masterTiles.tileDebugString(tileIdx) + ": matched palette " + std::to_string(j);
            verboseLog(logString);
            logString.clear();
            break;
        }
        if (j == palettes.size() - 1) {
            // If we made it here without triggering a palette assignment above, there's a problem
            throw std::runtime_error{"internal: could not allocate palette for tile " + std::to_string(tileIdx)};
        }
    }

    /*
     * When we find the palette for this tile, construct an indexed tile by comparing each `RgbColor` in the tile to
     * those in the palette vector and choosing the matches. Finally, check each transform of this `IndexedTile`: if it
     * is already in the tileset (flip X, flip Y, flip X and Y). If it is, skip. Otherwise, add it!
     */
    size_t idx = 0;
    for (const auto& color: tile.getPixels()) {
        size_t indexInPalette = palettes[matchingPaletteIndex].indexOf(color);
        indexedTile.setPixel(idx, indexInPalette);
        idx++;
    }

    const IndexedTile verticalFlip = indexedTile.getVerticalFlip();
    const IndexedTile horizontalFlip = indexedTile.getHorizontalFlip();
    const IndexedTile diagonalFlip = indexedTile.getDiagonalFlip();

    if (tilesIndex.find(indexedTile) != tilesIndex.end()) {
        logString = masterTiles.tileDebugString(tileIdx) + ": skipping: tile already present";
        verboseLog(logString);
        logString.clear();
        return;
    }

    if (tilesIndex.find(verticalFlip) != tilesIndex.end()) {
        logString = masterTiles.tileDebugString(tileIdx) + ": skipping: vertical flip already present";
        verboseLog(logString);
        logString.clear();
        return;
    }
    if (tilesIndex.find(horizontalFlip) != tilesIndex.end()) {
        logString = masterTiles.tileDebugString(tileIdx) + ": skipping: horizontal flip already present";
        verboseLog(logString);
        logString.clear();
        return;
    }
    if (tilesIndex.find(diagonalFlip) != tilesIndex.end()) {
        logString = masterTiles.tileDebugString(tileIdx) + ": skipping: diagonal flip already present";
        verboseLog(logString);
        logString.clear();
        return;
    }

    tiles.push_back(indexedTile);
    tilesIndex.insert(indexedTile);
    logString = masterTiles.tileDebugString(tileIdx) + ": mapped to final tile: 0x";
    size_t finalTileIdx = tiles.size() - 1;
    // convert tile index to hex
    std::stringstream sstream;
    sstream << std::hex << finalTileIdx;
    logString += sstream.str();
    logString += " (" + std::to_string(finalTileIdx % FINAL_IMAGE_WIDTH_IN_TILES) + "," +
                 std::to_string(finalTileIdx / FINAL_IMAGE_WIDTH_IN_TILES) + ")";
    verboseLog(logString);
    logString.clear();

    if (tiles.size() > gOptMaxTiles - 1) {
        /*
         * -1 to account for the transparent tile, which will be inserted at the beginning of the tile vector after all
         * other tiles have processed.
         */
        throw TsException{"requested tile limit (" + std::to_string(gOptMaxTiles) + ") was exceeded"};
    }
}

void emitPalette(size_t palIndex, const std::filesystem::path& basePath, const Palette& palette) {
    std::string fileName = palIndex < 10 ? "0" + std::to_string(palIndex) : std::to_string(palIndex);
    fileName += ".pal";
    const size_t maxPalSize = PAL_SIZE_4BPP + 1;

    verboseLog("writing palette " + fileName);
    std::string outpath = basePath / fileName;
    std::ofstream outfile{outpath};
    outfile << "JASC-PAL" << std::endl;
    outfile << "0100" << std::endl;
    outfile << "16" << std::endl;
    size_t i;
    for (i = 0; i < palette.size(); i++) {
        outfile << palette.colorAt(i).jascString() << std::endl;
    }
    for (size_t j = i; j < maxPalSize; j++) {
        outfile << RgbColor{0, 0, 0}.jascString() << std::endl;
    }
    outfile.close();
}
} // namespace (anonymous)

Tileset::Tileset(const size_t maxPalettes) : maxPalettes{maxPalettes} {
    palettes.reserve(this->maxPalettes);
    for (int i = 0; i < getMaxPalettes(); i++) {
        // prefill palette vector with empty palettes
        palettes.emplace_back();
    }
}

void Tileset::validateControlTileLayout(const RgbTiledPng& masterTiles) const {
    // TODO implement
}

void Tileset::alignSiblings(const RgbTiledPng& masterTiles) {
    std::string logString;
    verboseLog("--------------- ALIGNING SIBLINGS ---------------");

    // Sibling control tile must appear as the first tile in the PNG, otherwise
    if (!masterTiles.tileAt(0).isUniformly(gOptSiblingColor)) {
        verboseLog("no sibling tiles present, skipping ahead to build palettes");
        return;
    }

    size_t tileIndex = 1;
    while (!masterTiles.tileAt(tileIndex).isUniformly(gOptSiblingColor)) {
        size_t paletteIndex = tileIndex - 1;
        if (paletteIndex >= gOptMaxPalettes) {
            throw TsException{
                    "too many sibling tiles, count cannot exceed max palettes (" +
                    std::to_string(gOptMaxPalettes) + ")"};
        }

        logString += masterTiles.tileDebugString(tileIndex);
        logString += ": sibling tile detected";
        verboseLog(logString);
        logString.clear();

        std::unordered_set<RgbColor> pixelColors;
        for (size_t pixelIndex = 0; pixelIndex < PIXEL_COUNT; pixelIndex++) {
            RgbColor colorAt = masterTiles.tileAt(tileIndex).getPixel(pixelIndex);
            if (colorAt == gOptTransparentColor) {
                continue;
            }
            if (pixelColors.find(colorAt) != pixelColors.end()) {
                throw TsException{
                        masterTiles.tileDebugString(tileIndex) + ": sibling tile contained duplicate color at pixel " +
                        std::to_string(pixelIndex)};
            }
            palettes.at(paletteIndex).addColorAtEnd(colorAt);
            pixelColors.insert(colorAt);

            logString += masterTiles.tileDebugString(tileIndex);
            logString += ": pixel " + std::to_string(pixelIndex) + ": ";
            logString += "pushed color " + colorAt.prettyString();
            verboseLog(logString);
            logString.clear();
        }
        tileIndex++;
    }
}

void Tileset::buildPalettes(const RgbTiledPng& masterTiles) {
    verboseLog("--------------- BUILDING PALETTES ---------------");
    for (size_t i = 0; i < masterTiles.size(); i++) {
        // TODO : skip sibling tiles here, not necessary and makes logs confusing
        assignTileToPalette(masterTiles, static_cast<int>(i), palettes);
    }
}

void Tileset::indexTiles(const RgbTiledPng& masterTiles) {
    bool inPrimerBlock = false;
    bool inSiblingBlock = false;
    std::string logString;

    verboseLog("--------------- INDEXING TILES ---------------");
    // Add transparent color to first entry in each palette
    for (auto& palette: palettes) {
        palette.pushTransparencyColor();
    }

    // Insert transparent tile at the start
    IndexedTile transparent{0};
    tiles.push_back(transparent);
    tilesIndex.insert(transparent);

    // Iterate over each tile and assign it, skipping primer and sibling tiles
    for (size_t i = 0; i < masterTiles.size(); i++) {
        if (masterTiles.tileAt(static_cast<int>(i)).isUniformly(gOptPrimerColor)) {
            if (inSiblingBlock) {
                throw TsException{masterTiles.tileDebugString(i) + ": cannot nest primer block within sibling block"};
            }

            inPrimerBlock = !inPrimerBlock;
            logString = inPrimerBlock ? "entered" : "exited";
            logString += " primer block " + masterTiles.tileDebugString(i);
            verboseLog(logString);
            logString.clear();
            continue;
        }
        if (masterTiles.tileAt(static_cast<int>(i)).isUniformly(gOptSiblingColor)) {
            if (inPrimerBlock) {
                throw TsException{masterTiles.tileDebugString(i) + ": cannot nest sibling block within primer block"};
            }

            inSiblingBlock = !inSiblingBlock;
            logString = inSiblingBlock ? "entered" : "exited";
            logString += " sibling block " + masterTiles.tileDebugString(i);
            verboseLog(logString);
            logString.clear();
            continue;
        }

        if (!inPrimerBlock && !inSiblingBlock)
            indexTile(masterTiles, static_cast<int>(i), palettes, tiles, tilesIndex);
    }

    // If we reach end of tiles without closing a block, throw an error since this is probably a mistake
    if (inPrimerBlock) {
        throw TsException{"reached end of tiles with open primer block"};
    }
    if (inSiblingBlock) {
        throw TsException{"reached end of tiles with open sibling block"};
    }
}

void Tileset::writeTileset() {
    std::filesystem::path outputPath(gArgOutputPath);
    std::filesystem::path palettesDir("palettes");
    std::filesystem::path tilesetFile("tiles.png");
    std::filesystem::path tilesetPath = gArgOutputPath / tilesetFile;
    std::filesystem::path palettesPath = gArgOutputPath / palettesDir;

    verboseLog("--------------- WRITING FILES ---------------");

    if (std::filesystem::exists(tilesetPath) && !std::filesystem::is_regular_file(tilesetPath)) {
        throw TsException{"`" + tilesetPath.string() + "' exists in output directory but is not a file"};
    }
    if (std::filesystem::exists(palettesPath) && !std::filesystem::is_directory(palettesPath)) {
        throw TsException{"`" + palettesDir.string() + "' exists in output directory but is not a directory"};
    }
    std::filesystem::create_directories(palettesPath);

    /*
     * Set final tileset PNG dimensions based on the tile vector. We'll also set it to use a simple greyscale palette.
     */
    const size_t imageWidth = TILE_DIMENSION * FINAL_IMAGE_WIDTH_IN_TILES;
    const size_t imageHeight = TILE_DIMENSION * ((tiles.size() / FINAL_IMAGE_WIDTH_IN_TILES) + 1);
    png::image<png::index_pixel> tilesetPng{static_cast<png::uint_32>(imageWidth),
                                            static_cast<png::uint_32>(imageHeight)};
    png::palette pal(16);
    for (size_t i = 0; i < pal.size(); i++) {
        int color = i == 0 ? 0 : (static_cast<int>(i) * 16) - 1;
        pal[15 - i] = png::color(color, color, color);
    }
    tilesetPng.set_palette(pal);

    /*
     * Set up the tileset PNG based on the tiles list and then write it to `tiles.png`.
     */
    png::uint_32 tilesetPngTilesWidth = tilesetPng.get_width() / TILE_DIMENSION;
    png::uint_32 tilesetPngTilesHeight = tilesetPng.get_height() / TILE_DIMENSION;
    size_t tileIndex = 0;
    for (png::uint_32 tileRow = 0; tileRow < tilesetPngTilesHeight; tileRow++) {
        for (png::uint_32 tileCol = 0; tileCol < tilesetPngTilesWidth; tileCol++) {
            png::uint_32 pixelRowStart = tileRow * TILE_DIMENSION;
            png::uint_32 pixelColStart = tileCol * TILE_DIMENSION;
            for (png::uint_32 row = 0; row < TILE_DIMENSION; row++) {
                for (png::uint_32 col = 0; col < TILE_DIMENSION; col++) {
                    if (tileIndex < tiles.size()) {
                        tilesetPng[pixelRowStart + row][pixelColStart + col] = tiles.at(tileIndex).getPixel(row, col);
                    } else {
                        tilesetPng[pixelRowStart + row][pixelColStart + col] = tiles.at(0).getPixel(row, col);
                    }
                }
            }
            tileIndex++;
        }
    }
    verboseLog("writing tileset " + tilesetFile.string());
    tilesetPng.write(tilesetPath);

    /*
     * Finally, write the pal files.
     */
    for (size_t i = 0; i < palettes.size(); i++) {
        emitPalette(i, palettesPath, palettes[i]);
    }
}
} // namespace porytiles
