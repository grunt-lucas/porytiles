#include "rgb_tiled_png.h"
#include "tsexception.h"

#include <png.hpp>
#include <stdexcept>

namespace porytiles {
namespace {
void throwIfStructureContainsControlTiles(const StructureRegion& structure, const RgbTiledPng& png) {
    for (size_t row = structure.topRow; row <= structure.bottomRow; row++) {
        for (size_t col = structure.leftCol; col <= structure.rightCol; col++) {
            if (png.tileAt(row, col).isUniformly(gOptPrimerColor)) {
                throw TsException{"structure starting at tile (" + std::to_string(structure.leftCol) + "," +
                                  std::to_string(structure.topRow) + ") had unexpected primer control tile at (" +
                                  std::to_string(col) + "," + std::to_string(row) + ")"};
            }
            if (png.tileAt(row, col).isUniformly(gOptSiblingColor)) {
                throw TsException{"structure starting at tile (" + std::to_string(structure.leftCol) + "," +
                                  std::to_string(structure.topRow) + ") had unexpected sibling control tile at (" +
                                  std::to_string(col) + "," + std::to_string(row) + ")"};
            }
        }
    }
}
}

RgbTiledPng::RgbTiledPng(const png::image<png::rgb_pixel>& png) {
    width = png.get_width() / TILE_DIMENSION;
    height = png.get_height() / TILE_DIMENSION;

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
            addTile(tile);
        }
    }
}

void RgbTiledPng::addTile(const RgbTile& tile) {
    if (tiles.size() >= width * height) {
        throw std::runtime_error{"internal: TiledPng::addTile tried to add beyond image dimensions"};
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

std::string RgbTiledPng::tileDebugString(size_t index) const {
    return "tile " + std::to_string(index) + " (col,row) " +
           "(" + std::to_string(index % width) +
           "," + std::to_string(index / width) + ")";
}
} // namespace porytiles
