#include "tile.h"

namespace porytiles {
template<>
RgbTile::Tile() {
    for (size_t i = 0; i < PIXEL_COUNT; i++) {
        pixels[i] = RgbColor{0, 0, 0};
    }
}

template<>
IndexedTile::Tile() {
    for (size_t i = 0; i < PIXEL_COUNT; i++) {
        pixels[i] = 0;
    }
}

template<>
std::unordered_set<RgbColor> RgbTile::pixelsNotInPalette(const Palette& palette) const {
    std::unordered_set<RgbColor> uniquePixels = this->uniquePixels(gOptTransparentColor);
    std::unordered_set<RgbColor> paletteIndex = palette.getIndex();
    std::unordered_set<RgbColor> pixelsNotInPalette;
    std::copy_if(uniquePixels.begin(), uniquePixels.end(),
                 std::inserter(pixelsNotInPalette, pixelsNotInPalette.begin()),
                 [&paletteIndex](const RgbColor& needle) { return paletteIndex.find(needle) == paletteIndex.end(); });
    return pixelsNotInPalette;
}

template<>
std::unordered_set<png::byte> IndexedTile::pixelsNotInPalette(const Palette& palette) const {
    throw std::runtime_error{"internal: invalid operation IndexedTile::pixelsNotInPalette"};
}

doctest::String toString(const RgbTile& tile) {
    std::string tileString = "Tile<RgbColor>{" + tile.getPixel(0).prettyString() + "..." +
                             tile.getPixel(PIXEL_COUNT - 1).prettyString() + "}";
    return tileString.c_str();
}

doctest::String toString(const IndexedTile& tile) {
    std::string tileString = "Tile<png::byte>{" + std::to_string(tile.getPixel(0)) + "..." +
                             std::to_string(tile.getPixel(PIXEL_COUNT - 1)) + "}";
    return tileString.c_str();
}
} // namespace porytiles

/*
 * Test Cases
 */
TEST_CASE("Logically equivalent Tile<T> objects should be equivalent under op==") {
    SUBCASE("Logically equivalent RgbTiles should match") {
        porytiles::RgbTile tile1{{0, 0, 0}};
        porytiles::RgbTile tile2{{0, 0, 0}};
        CHECK(tile1 == tile2);
    }
    SUBCASE("Logically divergent RgbTiles should not match") {
        porytiles::RgbTile tile1{{0, 0, 0}};
        porytiles::RgbTile tile2{{1, 1, 1}};
        CHECK(tile1 != tile2);
    }
    SUBCASE("Logically equivalent IndexedTiles should match") {
        porytiles::IndexedTile tile1{0};
        porytiles::IndexedTile tile2{0};
        CHECK(tile1 == tile2);
    }
    SUBCASE("Logically divergent IndexedTiles should not match") {
        porytiles::IndexedTile tile1{0};
        porytiles::IndexedTile tile2{1};
        CHECK(tile1 != tile2);
    }
}

TEST_CASE("Tile<T> copy ctor should create a copy that is equivalent under op==") {
    SUBCASE("It should work for RgbTile") {
        porytiles::RgbTile tile1{{0, 0, 0}};
        porytiles::RgbTile tile2{tile1};
        CHECK(tile1 == tile2);
    }
    SUBCASE("It should work for IndexedTile") {
        porytiles::IndexedTile tile1{12};
        porytiles::IndexedTile tile2{tile1};
        CHECK(tile1 == tile2);
    }
}

TEST_CASE("Tile<T> getPixel should either return the requested pixel or throw on out-of-bounds") {
    porytiles::RgbTile zeroRgbTile{{0, 0, 0}};

    SUBCASE("It should return {0,0,0} for pixel row=0,col=0") {
        CHECK(zeroRgbTile.getPixel(0, 0) == porytiles::RgbColor{0, 0, 0});
    }
    SUBCASE("It should return {0,0,0} for pixel index=10") {
        CHECK(zeroRgbTile.getPixel(10) == porytiles::RgbColor{0, 0, 0});
    }
    SUBCASE("It should throw a std::out_of_range for out-of-bounds pixel index=1000") {
        porytiles::RgbColor dummy;
        CHECK_THROWS_WITH_AS(dummy = zeroRgbTile.getPixel(1000),
                             "internal: Tile::getPixel index argument out of bounds (1000)", const std::out_of_range&);
    }
    SUBCASE("It should throw a std::out_of_range for out-of-bounds pixel row=1000,col=0") {
        porytiles::RgbColor dummy;
        CHECK_THROWS_WITH_AS(dummy = zeroRgbTile.getPixel(1000, 0),
                             "internal: Tile::getPixel row argument out of bounds (1000)", const std::out_of_range&);
    }
    SUBCASE("It should throw a std::out_of_range for out-of-bounds pixel row=0,col=1000") {
        porytiles::RgbColor dummy;
        CHECK_THROWS_WITH_AS(dummy = zeroRgbTile.getPixel(0, 1000),
                             "internal: Tile::getPixel col argument out of bounds (1000)", const std::out_of_range&);
    }
}

TEST_CASE("Tile<T> setPixel should either return the requested pixel or throw on out-of-bounds") {
    porytiles::IndexedTile zeroIndexedTile{0};

    SUBCASE("It should set pixel row=1,col=0 to value 1") {
        zeroIndexedTile.setPixel(1, 0, 1);
        CHECK(zeroIndexedTile.getPixel(1, 0) == 1);
    }
    SUBCASE("It should set pixel index=22 to value 12") {
        zeroIndexedTile.setPixel(22, 12);
        CHECK(zeroIndexedTile.getPixel(22) == 12);
    }
    SUBCASE("It should throw a std::out_of_range for out-of-bounds pixel index=1000") {
        CHECK_THROWS_WITH_AS(zeroIndexedTile.setPixel(1000, 0),
                             "internal: Tile::setPixel index argument out of bounds (1000)", const std::out_of_range&);
    }
    SUBCASE("It should throw a std::out_of_range for out-of-bounds pixel row=1000,col=0") {
        CHECK_THROWS_WITH_AS(zeroIndexedTile.setPixel(1000, 0, 0),
                             "internal: Tile::setPixel row argument out of bounds (1000)", const std::out_of_range&);
    }
    SUBCASE("It should throw a std::out_of_range for out-of-bounds pixel row=0,col=1000") {
        CHECK_THROWS_WITH_AS(zeroIndexedTile.setPixel(0, 1000, 0),
                             "internal: Tile::setPixel col argument out of bounds (1000)", const std::out_of_range&);
    }
}

TEST_CASE("Tile<T> isUniformly should only be true if entire tile is the requested color") {
    porytiles::RgbColor red = {255, 0, 0};
    porytiles::RgbTile redTile{red};

    SUBCASE("It should detect that the tile is uniformly red") {
        CHECK(redTile.isUniformly(red));
    }
    SUBCASE("It should detect that the tile has a non-red pixel") {
        porytiles::RgbColor green = {0, 255, 0};
        redTile.setPixel(12, green);
        CHECK(!redTile.isUniformly(red));
    }
}

TEST_CASE("Tile<T> uniquePixels should show all unique colors not including the supplied transparency color") {
    porytiles::RgbColor red = {255, 0, 0};
    porytiles::RgbColor green = {0, 255, 0};
    porytiles::RgbColor blue = {0, 0, 255};
    porytiles::RgbColor transparent = {255, 0, 255};

    SUBCASE("It should return a set of size 2 containing red and blue") {
        porytiles::RgbTile tile{red};
        for (size_t row = 0; row < porytiles::TILE_DIMENSION; row++) {
            for (size_t col = 0; col < porytiles::TILE_DIMENSION; col++) {
                if (row % 2 == 0 && col % 2 == 0) {
                    tile.setPixel(row, col, green);
                }
            }
        }
        tile.setPixel(12, transparent);
        std::unordered_set<porytiles::RgbColor> uniqueColors = tile.uniquePixels(transparent);

        CHECK(uniqueColors.size() == 2);
        CHECK(uniqueColors.find(red) != uniqueColors.end());
        CHECK(uniqueColors.find(green) != uniqueColors.end());
    }
    SUBCASE("It should return a set of size 0 since tile was uniformly transparent") {
        porytiles::RgbTile tile{transparent};
        std::unordered_set<porytiles::RgbColor> uniqueColors = tile.uniquePixels(transparent);
        CHECK(uniqueColors.size() == 0);
    }
}

TEST_CASE("Tile<T> flip functions should correctly flip tile pixels across horizontal, vertical, or diagonal axes") {
    porytiles::IndexedTile tile{0};
    for (size_t index = 0; index < porytiles::PIXEL_COUNT; index++) {
        tile.setPixel(index, index);
    }

    SUBCASE("It should correctly flip the tile horizontally") {
        porytiles::IndexedTile horizontalFlip = tile.getHorizontalFlip();
        size_t counter = 0;
        for (size_t row = 0; row < porytiles::TILE_DIMENSION; row++) {
            for (int col = porytiles::TILE_DIMENSION - 1; col >= 0; col--) {
                CHECK(horizontalFlip.getPixel(row, static_cast<size_t>(col)) == counter);
                counter++;
            }
        }
    }
    SUBCASE("It should correctly flip the tile vertically") {
        porytiles::IndexedTile verticalFlip = tile.getVerticalFlip();
        size_t counter = 0;
        for (int row = porytiles::TILE_DIMENSION - 1; row >= 0; row--) {
            for (size_t col = 0; col < porytiles::TILE_DIMENSION; col++) {
                CHECK(verticalFlip.getPixel(static_cast<size_t>(row), col) == counter);
                counter++;
            }
        }
    }
    SUBCASE("It should correctly flip the tile diagonally") {
        porytiles::IndexedTile diagonalFlip = tile.getDiagonalFlip();
        size_t counter = 0;
        for (int row = porytiles::TILE_DIMENSION - 1; row >= 0; row--) {
            for (int col = porytiles::TILE_DIMENSION - 1; col >= 0; col--) {
                CHECK(diagonalFlip.getPixel(static_cast<size_t>(row), static_cast<size_t>(col)) == counter);
                counter++;
            }
        }
    }
    SUBCASE("Flipping tiles horizontally and vertically should be commutative") {
        porytiles::IndexedTile diagonalFlip1 = tile.getHorizontalFlip().getVerticalFlip();
        porytiles::IndexedTile diagonalFlip2 = tile.getVerticalFlip().getHorizontalFlip();
        porytiles::IndexedTile diagonalFlip3 = tile.getDiagonalFlip();

        CHECK(diagonalFlip1 == diagonalFlip2);
        CHECK(diagonalFlip2 == diagonalFlip3);
        CHECK(diagonalFlip3 == diagonalFlip1);
    }
}

TEST_CASE("RgbTile pixelsNotInPalette should return the pixels from this tile that are not in the supplied palette") {
    porytiles::RgbColor red = {255, 0, 0};
    porytiles::RgbColor green = {0, 255, 0};
    porytiles::RgbColor blue = {0, 0, 255};
    porytiles::RgbColor white = {255, 255, 255};
    porytiles::RgbColor black = {0, 0, 0};
    porytiles::RgbColor magenta = {255, 0, 255};
    porytiles::Palette palette{};
    palette.addColorAtEnd(red);
    palette.addColorAtEnd(green);
    palette.addColorAtEnd(blue);
    palette.pushFrontTransparencyColor();
    porytiles::RgbTile tile{magenta};

    SUBCASE("It should return a set of size 2 with colors black and white") {
        tile.setPixel(0, red);
        tile.setPixel(1, green);
        tile.setPixel(2, blue);
        tile.setPixel(3, black);
        tile.setPixel(4, white);

        std::unordered_set<porytiles::RgbColor> pixelsNotInPalette = tile.pixelsNotInPalette(palette);
        CHECK(pixelsNotInPalette.size() == 2);
        CHECK(pixelsNotInPalette.find(black) != pixelsNotInPalette.end());
        CHECK(pixelsNotInPalette.find(white) != pixelsNotInPalette.end());
    }
    SUBCASE("It should return a set of size 0") {
        tile.setPixel(0, red);
        tile.setPixel(1, green);
        tile.setPixel(2, blue);
        std::unordered_set<porytiles::RgbColor> pixelsNotInPalette = tile.pixelsNotInPalette(palette);
        CHECK(pixelsNotInPalette.empty());
    }
}
