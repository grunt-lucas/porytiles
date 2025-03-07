/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

package models

// GBAPalette is a palette of PalSize (16) BGR15-format colors. A GBAPalette has
// a fixed size of PalSize, which is the hardware size of a GBA palette.
type GBAPalette struct {
	// LogicalSize represents the "logical" size of the palette, since the true
	// size is fixed at 16, the GBA hardware palette size.
	LogicalSize int
	Colors      [PalSize]BGR15
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

// UniformGBATile generates a GBATile with all pixels set to the given value.
func UniformGBATile(value uint8) GBATile {
	var tile GBATile
	for i := range tile.ColorIndexes {
		tile.ColorIndexes[i] = value
	}
	return tile
}
