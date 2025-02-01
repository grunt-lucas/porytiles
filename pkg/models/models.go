/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

// Package models defines all the types that Porytiles understands.
package models

// PalSize defines the size of a palette. On the GBA, palettes contain 16
// colors.
const PalSize int = 16

// TileSideLengthPix defines the length of a tile side in pixels. Since tiles
// are always squares, the side length can also be used to compute the pixel
// count. On the GBA, in tile modes relevant to the Pokémon decomps, tiles are
// 8x8 pixels.
const TileSideLengthPix int = 8

// TilePixCount is the total number of pixels in a Pokémon decomps GBA tile. In
// this case, it will be 8x8 = 64.
const TilePixCount = TileSideLengthPix * TileSideLengthPix

// TileType describes the different variants of a tile. These types are general
// and can apply to any kind of tile.
type TileType int

const (
	// A Layered tile is the standard tile type for a tile from one of the layer
	// sheets in 'compile tileset' mode. Layered tiles are the building blocks
	// of a Metatile.
	Layered TileType = iota
	// A Free tile is a non-layered tile from one of the spritesheets in
	// 'compile spritesheet' mode.
	Free
	// An Anim tile is an animated tile (from the 'anim' folder).
	Anim
	// A Primer tile is a special kind of tile that isn't actually present in
	// any of the input PNGs. Rather, it's constructed from one of the palette
	// primer files, if present.
	Primer
)

// TilesPerMetatile returns the number of tiles per metatile depending on the
// prescribed layer configuration. There are four subtiles per metatile layer,
// so triple-layer metatiles contain 12 total tiles, while traditional
// dual-layer tiles contain 8.
func TilesPerMetatile(tripleLayer bool) int {
	if tripleLayer {
		return 12
	}
	return 8
}
