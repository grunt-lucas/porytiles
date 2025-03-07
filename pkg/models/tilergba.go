/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

package models

// RGBATile represents a tile with pixels in RGBA32 format. Each pixel is stored
// as a value of type RGBA32.
type RGBATile struct {
	Type   TileType
	Pixels [TilePixCount]RGBA32
}

// PixelAt retrieves the RGBA32 pixel at the specified row and column in the
// tile.
func (t *RGBATile) PixelAt(row, col int) RGBA32 {
	return t.Pixels[row*TileSideLengthPix+col]
}

// IsBGREquivalentTo checks if two RGBATiles are equivalent after conversion to
// BGR15 format.
func (t *RGBATile) IsBGREquivalentTo(other *RGBATile) bool {
	for i := range t.Pixels {
		if RGBAToBGR(t.Pixels[i]) != RGBAToBGR(other.Pixels[i]) {
			return false
		}
	}
	return true
}
