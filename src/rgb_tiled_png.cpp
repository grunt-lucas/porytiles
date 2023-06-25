#include "rgb_tiled_png.h"

#include <doctest.h>
#include <png.hpp>
#include <stdexcept>
#include <optional>
#include <filesystem>

#include "tsexception.h"
#include "tsoutput.h"

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

    // Set up structure control regions
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

    bool inPrimerRegion = false;
    bool inSiblingRegion = false;
    std::string logString;
    // Set up primer and sibling regions
    size_t tmpStart = 0;
    for (size_t tileIndex = 0; tileIndex < tiles.size(); tileIndex++) {
        if (tileAt(tileIndex).isUniformly(gOptPrimerColor)) {
            if (inSiblingRegion) {
                throw TsException{tileDebugString(tileIndex) + ": cannot nest primer region within sibling region"};
            }

            inPrimerRegion = !inPrimerRegion;
            logString = inPrimerRegion ? "entered" : "exited";
            logString += " primer region " + tileDebugString(tileIndex);
            verboseLog(logString);
            logString.clear();

            if (inPrimerRegion) {
                // Just entered primer region
                tmpStart = tileIndex;
            }
            else {
                // Exited primer region
                if (tileIndex == tmpStart + 1) {
                    // Primer region had no content, throw
                    throw TsException{tileDebugString(tmpStart) + ": primer region opened here had length 0"};
                }
                primers.push_back({tmpStart + 1, tileIndex - tmpStart - 1});
            }
            continue;
        }
        if (tileAt(tileIndex).isUniformly(gOptSiblingColor)) {
            if (inPrimerRegion) {
                throw TsException{tileDebugString(tileIndex) + ": cannot nest sibling region within primer region"};
            }

            inSiblingRegion = !inSiblingRegion;
            logString = inSiblingRegion ? "entered" : "exited";
            logString += " sibling region " + tileDebugString(tileIndex);
            verboseLog(logString);
            logString.clear();

            if (inSiblingRegion) {
                // Just entered sibling region
                if (tileIndex != 0) {
                    throw TsException{"sibling region must start at the first tile"};
                }
                tmpStart = tileIndex;
            }
            else {
                if (tileIndex == tmpStart + 1) {
                    // Primer region had no content, throw
                    throw TsException{tileDebugString(tmpStart) + ": sibling region opened here had length 0"};
                }
                // Exited sibling region
                siblings.push_back({tmpStart + 1, tileIndex - tmpStart - 1});
            }
            continue;
        }
    }
    // If we reach end of tiles without closing a region, throw an error since this is probably a mistake
    if (inPrimerRegion) {
        throw TsException{"reached end of tiles with open primer region"};
    }
    if (inSiblingRegion) {
        throw TsException{"reached end of tiles with open sibling region"};
    }
}

void RgbTiledPng::pushTile(const RgbTile& tile) {
    if (tiles.size() >= width * height) {
        throw std::length_error{"internal: TiledPng::pushTile tried to add beyond image dimensions"};
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

const std::vector<LinearRegion>& RgbTiledPng::getPrimerRegions() const {
    return primers;
}

std::optional<LinearRegion> RgbTiledPng::getSiblingRegion() const {
    if (!siblings.empty()) {
        return {siblings.at(0)};
    }
    return std::nullopt;
}

const std::vector<StructureRegion>& RgbTiledPng::getStructureRegions() const {
    return structures;
}

std::string RgbTiledPng::tileDebugString(size_t index) const {
    return "tile " + std::to_string(index) + " (col,row) " +
           "(" + std::to_string(index % width) +
           "," + std::to_string(index / width) + ")";
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
    size_t topRow = indexToRowCol(index).first;
    size_t leftCol = indexToRowCol(index).second;

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
                              std::to_string(leftCol) + "," + std::to_string(bottomRow) + ")"};
        }
        else if (tileAt(bottomRow, leftCol).isUniformly(gOptSiblingColor)) {
            throw TsException{"structure starting at tile (" + std::to_string(leftCol) + "," +
                              std::to_string(topRow) + ") had unexpected sibling control tile at (" +
                              std::to_string(leftCol) + "," + std::to_string(bottomRow) + ")"};
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
} // namespace porytiles

/*
 * Test Cases
 */
TEST_CASE("RgbTiledPng basic metadata methods should return correct results") {
    REQUIRE(std::filesystem::exists("res/tests/control_tiles_test_1.png"));
    png::image<png::rgb_pixel> png{"res/tests/control_tiles_test_1.png"};
    porytiles::RgbTiledPng tiledPng{png};

    SUBCASE("RgbTiledPng size method should return the number of tiles") {
        CHECK(tiledPng.size() == 64);
    }
    SUBCASE("RgbTiledPng getWidth method should return the width in tiles") {
        CHECK(tiledPng.getWidth() == 8);
    }
    SUBCASE("RgbTiledPng getHeight method should return the height in tiles") {
        CHECK(tiledPng.getHeight() == 8);
    }
}

TEST_CASE("RgbTiledPng pushTile should throw when adding past the set image dimensions") {
    REQUIRE(std::filesystem::exists("res/tests/control_tiles_test_1.png"));
    png::image<png::rgb_pixel> png{"res/tests/control_tiles_test_1.png"};
    porytiles::RgbTiledPng tiledPng{png};

    CHECK(tiledPng.size() == 64);

    porytiles::RgbTile transparentTile{porytiles::gOptTransparentColor};

    CHECK_THROWS_WITH_AS(tiledPng.pushTile(transparentTile),
                         "internal: TiledPng::pushTile tried to add beyond image dimensions", const std::length_error&);
}

TEST_CASE("RgbTiledPng tileAt variations should correctly return the requested tile") {
    REQUIRE(std::filesystem::exists("res/tests/control_tiles_test_1.png"));
    png::image<png::rgb_pixel> png{"res/tests/control_tiles_test_1.png"};
    porytiles::RgbTiledPng tiledPng{png};

    SUBCASE("tileAt(row, col) should return the correct tile") {
        CHECK(tiledPng.tileAt(2, 0).isUniformly(porytiles::gOptTransparentColor));
        CHECK(tiledPng.tileAt(7, 3).isUniformly(porytiles::gOptTransparentColor));
    }
    SUBCASE("tileAt(index) should return the correct tile") {
        CHECK(tiledPng.tileAt(16).isUniformly(porytiles::gOptTransparentColor));
        CHECK(tiledPng.tileAt(48).isUniformly(porytiles::gOptTransparentColor));
    }
    SUBCASE("tileAt(row, col) should equal tileAt(index) for corresponding row,col and index") {
        CHECK(tiledPng.tileAt(1, 0) == tiledPng.tileAt(8));
        CHECK(tiledPng.tileAt(4, 2) == tiledPng.tileAt(34));
    }
}

TEST_CASE("RgbTiledPng indexToRowCol should return the correct row,col for the given index") {
    REQUIRE(std::filesystem::exists("res/tests/control_tiles_test_1.png"));
    png::image<png::rgb_pixel> png{"res/tests/control_tiles_test_1.png"};
    porytiles::RgbTiledPng tiledPng{png};

    CHECK(tiledPng.tileAt(tiledPng.indexToRowCol(8).first, tiledPng.indexToRowCol(8).second) == tiledPng.tileAt(8));
    CHECK(tiledPng.tileAt(tiledPng.indexToRowCol(9).first, tiledPng.indexToRowCol(9).second) == tiledPng.tileAt(9));
    CHECK(tiledPng.tileAt(tiledPng.indexToRowCol(10).first, tiledPng.indexToRowCol(10).second) == tiledPng.tileAt(10));
    CHECK(tiledPng.tileAt(tiledPng.indexToRowCol(33).first, tiledPng.indexToRowCol(33).second) == tiledPng.tileAt(33));
    CHECK(tiledPng.tileAt(tiledPng.indexToRowCol(34).first, tiledPng.indexToRowCol(34).second) == tiledPng.tileAt(34));
}

TEST_CASE("RgbTiledPng rowColToIndex should return the correct index for the given row,col") {
    REQUIRE(std::filesystem::exists("res/tests/control_tiles_test_1.png"));
    png::image<png::rgb_pixel> png{"res/tests/control_tiles_test_1.png"};
    porytiles::RgbTiledPng tiledPng{png};

    CHECK(tiledPng.tileAt(tiledPng.rowColToIndex(1, 0)) == tiledPng.tileAt(1, 0));
    CHECK(tiledPng.tileAt(tiledPng.rowColToIndex(1, 1)) == tiledPng.tileAt(1, 1));
    CHECK(tiledPng.tileAt(tiledPng.rowColToIndex(1, 2)) == tiledPng.tileAt(1, 2));
    CHECK(tiledPng.tileAt(tiledPng.rowColToIndex(4, 1)) == tiledPng.tileAt(4, 1));
    CHECK(tiledPng.tileAt(tiledPng.rowColToIndex(4, 2)) == tiledPng.tileAt(4, 2));
}

TEST_CASE("RgbTiledPng getPrimerRegions should return the correct primer regions or fail with correct error") {
    REQUIRE(std::filesystem::exists("res/tests/control_tiles_test_1.png"));
    REQUIRE(std::filesystem::exists("res/tests/15_colors_in_tile_plus_transparency.png"));
    png::image<png::rgb_pixel> png1{"res/tests/control_tiles_test_1.png"};
    png::image<png::rgb_pixel> png2{"res/tests/15_colors_in_tile_plus_transparency.png"};

    porytiles::RgbTiledPng tiledPng{png1};
    const std::vector<porytiles::LinearRegion>& primerRegions = tiledPng.getPrimerRegions();

    CHECK(primerRegions.size() == 2);
    CHECK(primerRegions.at(0).size == 1);
    CHECK(primerRegions.at(0).startIndex == 25);
    CHECK(primerRegions.at(1).size == 1);
    CHECK(primerRegions.at(1).startIndex == 57);

    porytiles::RgbTiledPng tiledPngNoPrimers{png2};
    const std::vector<porytiles::LinearRegion>& noPrimerRegions = tiledPngNoPrimers.getPrimerRegions();
    CHECK(noPrimerRegions.size() == 0);
}

TEST_CASE("RgbTiledPng getSiblingRegions should return the correct sibling region or fail with correct error") {
    REQUIRE(std::filesystem::exists("res/tests/control_tiles_test_1.png"));
    REQUIRE(std::filesystem::exists("res/tests/15_colors_in_tile_plus_transparency.png"));
    png::image<png::rgb_pixel> png1{"res/tests/control_tiles_test_1.png"};
    png::image<png::rgb_pixel> png2{"res/tests/15_colors_in_tile_plus_transparency.png"};

    porytiles::RgbTiledPng tiledPngWithSiblings{png1};
    const std::optional<porytiles::LinearRegion>& siblingRegion = tiledPngWithSiblings.getSiblingRegion();
    CHECK(siblingRegion);
    CHECK(siblingRegion->size == 2);
    CHECK(siblingRegion->startIndex == 1);

    porytiles::RgbTiledPng tiledPngNoSiblings{png2};
    const std::optional<porytiles::LinearRegion>& noSiblingRegion = tiledPngNoSiblings.getSiblingRegion();
    CHECK(!noSiblingRegion);
}

TEST_CASE("RgbTiledPng getStructureRegions should return the correct structure regions or fail with correct error") {
    REQUIRE(std::filesystem::exists("res/tests/control_tiles_test_1.png"));
    REQUIRE(std::filesystem::exists("res/tests/15_colors_in_tile_plus_transparency.png"));
    REQUIRE(std::filesystem::exists("res/tests/invalid_structure_width_less_than_3.png"));
    REQUIRE(std::filesystem::exists("res/tests/invalid_structure_height_less_than_3.png"));
    REQUIRE(std::filesystem::exists("res/tests/invalid_structure_primer_tile_in_top.png"));
    REQUIRE(std::filesystem::exists("res/tests/invalid_structure_primer_tile_in_left.png"));
    REQUIRE(std::filesystem::exists("res/tests/invalid_structure_sibling_tile_in_top.png"));
    REQUIRE(std::filesystem::exists("res/tests/invalid_structure_sibling_tile_in_left.png"));
    REQUIRE(std::filesystem::exists("res/tests/invalid_structure_broken_right_border.png"));
    REQUIRE(std::filesystem::exists("res/tests/invalid_structure_broken_bottom_border.png"));
    REQUIRE(std::filesystem::exists("res/tests/invalid_structure_contains_primer_tile.png"));
    REQUIRE(std::filesystem::exists("res/tests/invalid_structure_contains_sibling_tile.png"));
    REQUIRE(std::filesystem::exists("res/tests/invalid_structure_contains_structure_tile.png"));

    SUBCASE("It should find the structure region and return the correct coordinates") {
        png::image<png::rgb_pixel> png{"res/tests/control_tiles_test_1.png"};
        porytiles::RgbTiledPng tiledPng{png};
        const std::vector<porytiles::StructureRegion>& structureRegions = tiledPng.getStructureRegions();
        CHECK(structureRegions.size() == 1);
        CHECK(structureRegions.at(0).topRow == 5);
        CHECK(structureRegions.at(0).leftCol == 4);
        CHECK(structureRegions.at(0).bottomRow == 7);
        CHECK(structureRegions.at(0).rightCol == 7);
    }

    SUBCASE("It should not find any structure regions") {
        png::image<png::rgb_pixel> png{"res/tests/15_colors_in_tile_plus_transparency.png"};
        porytiles::RgbTiledPng tiledPng{png};
        const std::vector<porytiles::StructureRegion>& structureRegions = tiledPng.getStructureRegions();
        CHECK(structureRegions.size() == 0);
    }

    SUBCASE("It should find an invalid structure region due to width < 3") {
        png::image<png::rgb_pixel> png{"res/tests/invalid_structure_width_less_than_3.png"};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        CHECK_THROWS_WITH_AS(porytiles::RgbTiledPng tiledPng{png},
                             "structure starting at tile (1,1) must have width of at least 3",
                             const porytiles::TsException&);
#pragma GCC diagnostic pop
    }

    SUBCASE("It should find an invalid structure region due to height < 3") {
        png::image<png::rgb_pixel> png{"res/tests/invalid_structure_height_less_than_3.png"};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        CHECK_THROWS_WITH_AS(porytiles::RgbTiledPng tiledPng{png},
                             "structure starting at tile (1,1) must have height of at least 3",
                             const porytiles::TsException&);
#pragma GCC diagnostic pop
    }

    SUBCASE("It should find an invalid structure region due to primer tile in top border") {
        png::image<png::rgb_pixel> png{"res/tests/invalid_structure_primer_tile_in_top.png"};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        CHECK_THROWS_WITH_AS(porytiles::RgbTiledPng tiledPng{png},
                             "structure starting at tile (1,1) had unexpected primer control tile at (3,1)",
                             const porytiles::TsException&);
#pragma GCC diagnostic pop
    }

    SUBCASE("It should find an invalid structure region due to primer tile in left border") {
        png::image<png::rgb_pixel> png{"res/tests/invalid_structure_primer_tile_in_left.png"};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        CHECK_THROWS_WITH_AS(porytiles::RgbTiledPng tiledPng{png},
                             "structure starting at tile (1,1) had unexpected primer control tile at (1,3)",
                             const porytiles::TsException&);
#pragma GCC diagnostic pop
    }

    SUBCASE("It should find an invalid structure region due to sibling tile in top border") {
        png::image<png::rgb_pixel> png{"res/tests/invalid_structure_sibling_tile_in_top.png"};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        CHECK_THROWS_WITH_AS(porytiles::RgbTiledPng tiledPng{png},
                             "structure starting at tile (1,1) had unexpected sibling control tile at (3,1)",
                             const porytiles::TsException&);
#pragma GCC diagnostic pop
    }

    SUBCASE("It should find an invalid structure region due to sibling tile in left border") {
        png::image<png::rgb_pixel> png{"res/tests/invalid_structure_sibling_tile_in_left.png"};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        CHECK_THROWS_WITH_AS(porytiles::RgbTiledPng tiledPng{png},
                             "structure starting at tile (1,1) had unexpected sibling control tile at (1,3)",
                             const porytiles::TsException&);
#pragma GCC diagnostic pop
    }

    SUBCASE("It should find an invalid structure region due to broken right border") {
        png::image<png::rgb_pixel> png{"res/tests/invalid_structure_broken_right_border.png"};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        CHECK_THROWS_WITH_AS(porytiles::RgbTiledPng tiledPng{png},
                             "structure starting at tile (1,1) had broken border at (6,4)",
                             const porytiles::TsException&);
#pragma GCC diagnostic pop
    }

    SUBCASE("It should find an invalid structure region due to broken bottom border") {
        png::image<png::rgb_pixel> png{"res/tests/invalid_structure_broken_bottom_border.png"};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        CHECK_THROWS_WITH_AS(porytiles::RgbTiledPng tiledPng{png},
                             "structure starting at tile (1,1) had broken border at (3,5)",
                             const porytiles::TsException&);
#pragma GCC diagnostic pop
    }

    SUBCASE("It should find an invalid structure region due to region containing primer tile") {
        png::image<png::rgb_pixel> png{"res/tests/invalid_structure_contains_primer_tile.png"};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        CHECK_THROWS_WITH_AS(porytiles::RgbTiledPng tiledPng{png},
                             "structure starting at tile (1,1) had unexpected primer control tile at (3,2)",
                             const porytiles::TsException&);
#pragma GCC diagnostic pop
    }

    SUBCASE("It should find an invalid structure region due to region containing sibling tile") {
        png::image<png::rgb_pixel> png{"res/tests/invalid_structure_contains_sibling_tile.png"};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        CHECK_THROWS_WITH_AS(porytiles::RgbTiledPng tiledPng{png},
                             "structure starting at tile (1,1) had unexpected sibling control tile at (4,3)",
                             const porytiles::TsException&);
#pragma GCC diagnostic pop
    }

    SUBCASE("It should find an invalid structure region due to region containing structure tile") {
        png::image<png::rgb_pixel> png{"res/tests/invalid_structure_contains_structure_tile.png"};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
        CHECK_THROWS_WITH_AS(porytiles::RgbTiledPng tiledPng{png},
                             "structure starting at tile (1,1) had unexpected structure control tile at (2,3)",
                             const porytiles::TsException&);
#pragma GCC diagnostic pop
    }
}
