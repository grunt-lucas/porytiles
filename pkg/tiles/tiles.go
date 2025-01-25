/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

// Package tiles defines the tile types that Porytiles understands, as well as
// various supporting types.
package tiles

import (
	"github.com/grunt-lucas/porytiles/pkg/colors"
)

// PalSize defines the size of a palette. On the GBA, palettes contain 16
// colors.
const PalSize int = 16

// GBAPalette is a palette of PalSize (16) BGR15-format colors. A GBAPalette has
// a fixed size of PalSize, which is the hardware size of a GBA palette.
type GBAPalette struct {
	// LogicalSize represents the "logical" size of the palette, since the true
	// size is fixed at 16, the GBA hardware palette size.
	LogicalSize int
	Colors      [PalSize]colors.BGR15
}

// TileSideLengthPix defines the length of a tile side in pixels. Since tiles
// are always squares, the side length can also be used to compute the pixel
// count. On the GBA, in tile modes relevant to the Pokémon decomps, tiles are
// 8x8 pixels.
const TileSideLengthPix int = 8

// TilePixCount is the total number of pixels in a Pokémon decomps GBA tile. In
// this case, it will be 8x8 = 64.
const TilePixCount = TileSideLengthPix * TileSideLengthPix

// RGBATile represents a tile with pixels in RGBA32 format. Each pixel is stored
// as a value of type colors.Rgba32.
type RGBATile struct {
	Pixels [TilePixCount]colors.RGBA32
}

// PixelAt retrieves the RGBA32 pixel at the specified row and column in the
// tile.
func (t *RGBATile) PixelAt(row, col int) colors.RGBA32 {
	return t.Pixels[row*TileSideLengthPix+col]
}

// IsBGREquivalentTo checks if two RGBATiles are equivalent after conversion to
// BGR15 format.
func (t *RGBATile) IsBGREquivalentTo(other *RGBATile) bool {
	for i := range t.Pixels {
		if colors.RGBAToBGR(t.Pixels[i]) != colors.RGBAToBGR(other.Pixels[i]) {
			return false
		}
	}
	return true
}

// GBATile represents a tile in GBA VRAM format. Each pixel is an index into a
// 16 color palette. A GBATile does not specify palette or flip information.
// That is specified at the Metatile level.
type GBATile struct {
	ColorIndexes [TilePixCount]uint8
}

// PixelAt retrieves the color index pixel at the specified row and column in
// the tile.
func (t *GBATile) PixelAt(row, col int) uint8 {
	return t.ColorIndexes[row*TileSideLengthPix+col]
}

// UniformTile generates a GbaTile with all pixels set to the given value pixel.
func UniformTile(value uint8) GBATile {
	var tile GBATile
	for i := range tile.ColorIndexes {
		tile.ColorIndexes[i] = value
	}
	return tile
}

// NormalizedPixels is a wrapper type for an array of 8-bit index pixels in
// normal form.
type NormalizedPixels struct {
	ColorIndexes [TilePixCount]uint8
}

// NormalizedPalette is a wrapper type for a slice of colors.BGR15 colors.
// NormalizedPalette is a logical representation, unlike GBAPalette, so it does
// not have a fixed size.
type NormalizedPalette struct {
	Colors []colors.BGR15
}

// NormalizedTileType defines the different variants of a NormalizedTile.
type NormalizedTileType int

const (
	// A Layered tile is the standard tile type for a tile from one of the layer
	// sheets in 'compile tileset' mode.
	Layered NormalizedTileType = iota
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

// A NormalizedTile is an individual 8x8 tile in normal form. This is the
// canonical intermediate tile representation for Porytiles.
type NormalizedTile struct {
	Type     NormalizedTileType
	KeyFrame NormalizedPixels
	Frames   []NormalizedPixels
	Palette  NormalizedPalette
	HFlip    bool
	VFlip    bool
}

// NewNormalizedTile creates and returns a new instance of a NormalizedTile. The
// NormalizedTile palette will contain the given transparent color in the first
// slot.
func NewNormalizedTile(transparentColor colors.RGBA32) *NormalizedTile {
	normTile := &NormalizedTile{}
	c := colors.RGBAToBGR(transparentColor)
	normTile.Palette.Colors = append(normTile.Palette.Colors, c)
	return normTile
}

// IsTransparent reports whether the given NormalizedTile is an entirely
// transparent tile.
func (n *NormalizedTile) IsTransparent() bool {
	return len(n.Palette.Colors) == 1
}
