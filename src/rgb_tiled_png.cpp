#include "rgb_tiled_png.h"

#include "tsexception.h"
#include "tsoutput.h"

#include <png.hpp>
#include <stdexcept>

namespace porytiles {

// Private helper methods
namespace {
void throwIfStructureContainsControlTiles(const StructureRegion& structure, const RgbTiledPng& png) {
    size_t topRow = structure.topRow;
    size_t bottomRow = structure.bottomRow;
    size_t leftCol = structure.leftCol;
    size_t rightCol = structure.rightCol;
    for (size_t row = topRow; row <= bottomRow; row++) {
        for (size_t col = leftCol; col <= rightCol; col++) {
            if (png.tileAt(row, col).isUniformly(gOptPrimerColor)) {
                throw TsException{"structure starting at tile (" + std::to_string(leftCol) + "," +
                                  std::to_string(topRow) + ") had unexpected primer control tile at (" +
                                  std::to_string(col) + "," + std::to_string(row) + ")"};
            }
            if (png.tileAt(row, col).isUniformly(gOptSiblingColor)) {
                throw TsException{"structure starting at tile (" + std::to_string(leftCol) + "," +
                                  std::to_string(topRow) + ") had unexpected sibling control tile at (" +
                                  std::to_string(col) + "," + std::to_string(row) + ")"};
            }
            if (png.tileAt(row, col).isUniformly(gOptStructureColor) && row != topRow && row != bottomRow &&
                col != leftCol && col != rightCol) {
                throw TsException{"structure starting at tile (" + std::to_string(leftCol) + "," +
                                  std::to_string(topRow) + ") had unexpected structure control tile at (" +
                                  std::to_string(col) + "," + std::to_string(row) + ")"};
            }
        }
    }
}
} // namespace (anonymous)
// END Private helper methods

RgbTiledPng::RgbTiledPng(const png::image<png::rgb_pixel>& png) {
    width = png.get_width() / TILE_DIMENSION;
    height = png.get_height() / TILE_DIMENSION;

    // Copy RGB values from the libpng PNG into this object's buffer
    for (size_t tileRow = 0; tileRow < height; tileRow++) {
        for (size_t tileCol = 0; tileCol < width; tileCol++) {
            long pixelRowStart = tileRow * TILE_DIMENSION;
            long pixelColStart = tileCol * TILE_DIMENSION;
            RgbTile tile{{0, 0, 0}};
            for (size_t row = 0; row < TILE_DIMENSION; row++) {
                for (size_t col = 0; col < TILE_DIMENSION; col++) {
                    png::byte red = png[pixelRowStart + row][pixelColStart + col].red;
                    png::byte green = png[pixelRowStart + row][pixelColStart + col].green;
                    png::byte blue = png[pixelRowStart + row][pixelColStart + col].blue;
                    tile.setPixel(row, col, RgbColor{red, green, blue});
                }
            }
            pushTile(tile);
        }
    }

    // Set up control tile regions
    std::unordered_set<size_t> processedIndexes;
    for (size_t tileIndex = 0; tileIndex < tiles.size(); tileIndex++) {
        if (processedIndexes.find(tileIndex) != processedIndexes.end()) {
            continue;
        }
        if (tileAt(tileIndex).isUniformly(gOptStructureColor)) {
            StructureRegion structure = getStructureStartingAt(tileIndex);
            structures.push_back(structure);
            for (size_t row = structure.topRow; row <= structure.bottomRow; row++) {
                for (size_t col = structure.leftCol; col <= structure.rightCol; col++) {
                    processedIndexes.insert(rowColToIndex(row, col));
                }
            }
        }
    }

    bool inPrimerBlock = false;
    bool inSiblingBlock = false;
    std::string logString;
    // Set up primer and sibling regions
    // TODO : this code allows zero-length primer/sibling blocks, we don't want this, should throw
    size_t tmpStart;
    for (size_t tileIndex = 0; tileIndex < tiles.size(); tileIndex++) {
        if (tileAt(tileIndex).isUniformly(gOptPrimerColor)) {
            if (inSiblingBlock) {
                throw TsException{tileDebugString(tileIndex) + ": cannot nest primer block within sibling block"};
            }

            inPrimerBlock = !inPrimerBlock;
            logString = inPrimerBlock ? "entered" : "exited";
            logString += " primer block " + tileDebugString(tileIndex);
            verboseLog(logString);
            logString.clear();

            if (inPrimerBlock) {
                // Just entered primer block
                tmpStart = tileIndex;
            }
            else {
                // Exited primer block
                primers.push_back({tmpStart, tileIndex});
            }
            continue;
        }
        if (tileAt(tileIndex).isUniformly(gOptSiblingColor)) {
            if (inPrimerBlock) {
                throw TsException{tileDebugString(tileIndex) + ": cannot nest sibling block within primer block"};
            }

            inSiblingBlock = !inSiblingBlock;
            logString = inSiblingBlock ? "entered" : "exited";
            logString += " sibling block " + tileDebugString(tileIndex);
            verboseLog(logString);
            logString.clear();

            if (inSiblingBlock) {
                // Just entered sibling block
                if (tileIndex != 0) {
                    throw TsException{"sibling block must start at the first tile"};
                }
                tmpStart = tileIndex;
            }
            else {
                // Exited sibling block
                siblings.push_back({tmpStart, tileIndex, tileIndex - tmpStart - 1});
            }
            continue;
        }
    }
    // If we reach end of tiles without closing a block, throw an error since this is probably a mistake
    if (inPrimerBlock) {
        throw TsException{"reached end of tiles with open primer block"};
    }
    if (inSiblingBlock) {
        throw TsException{"reached end of tiles with open sibling block"};
    }
}

void RgbTiledPng::pushTile(const RgbTile& tile) {
    if (tiles.size() >= width * height) {
        throw std::runtime_error{"internal: TiledPng::pushTile tried to add beyond image dimensions"};
    }
    tiles.push_back(tile);
}

const RgbTile& RgbTiledPng::tileAt(size_t row, size_t col) const {
    if (row >= height)
        throw std::runtime_error{
                "internal: RgbTiledPng::tileAt row argument out of bounds (" + std::to_string(row) + ")"};
    if (col >= width)
        throw std::runtime_error{
                "internal: RgbTiledPng::tileAt col argument out of bounds (" + std::to_string(col) + ")"};
    return tiles.at(row * width + col);
}

const RgbTile& RgbTiledPng::tileAt(size_t index) const {
    if (index >= width * height) {
        throw std::runtime_error{"internal: TiledPng::tileAt tried to reference index beyond image dimensions: " +
                                 std::to_string(index)};
    }
    return tiles.at(index);
}

std::pair<size_t, size_t> RgbTiledPng::indexToRowCol(size_t index) const {
    if (index >= tiles.size()) {
        throw std::runtime_error{
                "index (" + std::to_string(index) + ") was >= tiles.size() = " + std::to_string(tiles.size())};
    }
    size_t row = index / width;
    size_t col = index % width;
    return std::pair{row, col};
}

size_t RgbTiledPng::rowColToIndex(size_t row, size_t col) const {
    if (row >= height)
        throw std::runtime_error{
                "internal: RgbTiledPng::rowColToIndex row argument out of bounds (" + std::to_string(row) + ")"};
    if (col >= width)
        throw std::runtime_error{
                "internal: RgbTiledPng::rowColToIndex col argument out of bounds (" + std::to_string(col) + ")"};
    size_t index = row * width;
    index += col;
    return index;
}

StructureRegion RgbTiledPng::getStructureStartingAt(size_t index) const {
    if (index >= tiles.size()) {
        throw std::runtime_error{
                "index (" + std::to_string(index) + ") was >= tiles.size() = " + std::to_string(tiles.size())};
    }
    if (!tiles.at(index).isUniformly(gOptStructureColor)) {
        throw std::runtime_error{
                "index (" + std::to_string(index) + ") was not a structure control tile"};
    }

    auto [topRow, leftCol] = indexToRowCol(index);

    // Find the right-side border
    size_t rightCol = leftCol;
    while (rightCol < width) {
        if (tileAt(topRow, rightCol).isUniformly(gOptStructureColor)) {
            rightCol++;
        }
        else if (tileAt(topRow, rightCol).isUniformly(gOptPrimerColor)) {
            throw TsException{"structure starting at tile (" + std::to_string(leftCol) + "," +
                              std::to_string(topRow) + ") had unexpected primer control tile at (" +
                              std::to_string(rightCol) + "," + std::to_string(topRow) + ")"};
        }
        else if (tileAt(topRow, rightCol).isUniformly(gOptSiblingColor)) {
            throw TsException{"structure starting at tile (" + std::to_string(leftCol) + "," +
                              std::to_string(topRow) + ") had unexpected sibling control tile at (" +
                              std::to_string(rightCol) + "," + std::to_string(topRow) + ")"};
        }
        else if (tileAt(topRow, rightCol).isUniformly(gOptTransparentColor)) {
            rightCol--;
            break;
        }
    }
    if (rightCol == width) {
        // Went off the right side without hitting transparent tile, decrement back and end
        rightCol--;
    }
    // rightCol must allow at least one tile of gap from leftCol, otherwise fail
    if (rightCol <= leftCol + 1) {
        throw TsException{"structure starting at tile (" + std::to_string(leftCol) + "," +
                          std::to_string(topRow) + ") must have width of at least 3"};
    }

    // Find the bottom border
    size_t bottomRow = topRow;
    while (bottomRow < height) {
        if (tileAt(bottomRow, leftCol).isUniformly(gOptStructureColor)) {
            bottomRow++;
        }
        else if (tileAt(bottomRow, leftCol).isUniformly(gOptPrimerColor)) {
            throw TsException{"structure starting at tile (" + std::to_string(leftCol) + "," +
                              std::to_string(topRow) + ") had unexpected primer control tile at (" +
                              std::to_string(rightCol) + "," + std::to_string(topRow) + ")"};
        }
        else if (tileAt(bottomRow, leftCol).isUniformly(gOptSiblingColor)) {
            throw TsException{"structure starting at tile (" + std::to_string(leftCol) + "," +
                              std::to_string(topRow) + ") had unexpected sibling control tile at (" +
                              std::to_string(rightCol) + "," + std::to_string(topRow) + ")"};
        }
        else if (tileAt(bottomRow, leftCol).isUniformly(gOptTransparentColor)) {
            bottomRow--;
            break;
        }
    }
    if (bottomRow == height) {
        // Went off the bottom without hitting transparent tile, decrement back and end
        bottomRow--;
    }
    // bottomRow must allow at least one tile of gap from topRow, otherwise fail
    if (bottomRow <= topRow + 1) {
        throw TsException{"structure starting at tile (" + std::to_string(leftCol) + "," +
                          std::to_string(topRow) + ") must have height of at least 3"};
    }

    // We've computed the four corners, but let's still validate that the user drew a complete square
    size_t rowColIter = topRow;
    while (rowColIter <= bottomRow) {
        // Throw if the right-side border is broken
        if (!tileAt(rowColIter, rightCol).isUniformly(gOptStructureColor)) {
            throw TsException{"structure starting at tile (" + std::to_string(leftCol) + "," +
                              std::to_string(topRow) + ") had broken border at (" + std::to_string(rightCol) + "," +
                              std::to_string(rowColIter) + ")"};
        }
        rowColIter++;
    }
    rowColIter = leftCol;
    while (rowColIter <= rightCol) {
        // Throw if the bottom border is broken
        if (!tileAt(bottomRow, rowColIter).isUniformly(gOptStructureColor)) {
            throw TsException{"structure starting at tile (" + std::to_string(leftCol) + "," +
                              std::to_string(topRow) + ") had broken border at (" + std::to_string(rowColIter) + "," +
                              std::to_string(bottomRow) + ")"};
        }
        rowColIter++;
    }

    StructureRegion structure{topRow, bottomRow, leftCol, rightCol};
    throwIfStructureContainsControlTiles(structure, *this);
    return structure;
}

const std::vector<LinearRegion>& RgbTiledPng::getPrimers() const {
    return primers;
}

const std::vector<LinearRegion>& RgbTiledPng::getSiblings() const {
    return siblings;
}

const std::vector<StructureRegion>& RgbTiledPng::getStructures() const {
    return structures;
}

std::string RgbTiledPng::tileDebugString(size_t index) const {
    return "tile " + std::to_string(index) + " (col,row) " +
           "(" + std::to_string(index % width) +
           "," + std::to_string(index / width) + ")";
}
} // namespace porytiles
