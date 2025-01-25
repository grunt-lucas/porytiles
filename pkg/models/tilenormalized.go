/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

package models

import (
	"github.com/grunt-lucas/porytiles/pkg/diagnostics"
)

// NormalizedPixels is a wrapper type for an array of 8-bit index pixels in
// normal form.
type NormalizedPixels struct {
	ColorIndexes [TilePixCount]uint8
}

// NormalizedPalette is a wrapper type for a slice of BGR15 colors.
// NormalizedPalette is a logical representation, unlike GBAPalette, so it does
// not have a fixed size.
type NormalizedPalette struct {
	Colors []BGR15
}

type InsertRGBAOpts struct {
	Transparent     RGBA32
	SourceRGB       *map[BGR15]RGBA32
	Engine          *diagnostics.DiagnosticEngine
	PixelRow        int
	PixelCol        int
	EmitDiagnostics bool
}

func (p *NormalizedPalette) InsertRGBA(rgba RGBA32, diagCtx *InsertRGBAOpts) {
	// transparentBGR := colors.RGBAToBGR(transparent)
}

// A NormalizedTile is an individual 8x8 tile in normal form. This is the
// canonical intermediate tile representation for Porytiles.
type NormalizedTile struct {
	Source  *RGBATile
	Frames  []NormalizedPixels
	Palette NormalizedPalette
	HFlip   bool
	VFlip   bool
}

// NewNormalizedTile creates and returns a new instance of a NormalizedTile. The
// NormalizedTile palette will contain the given transparent color in the first
// slot.
func NewNormalizedTile(transparentColor RGBA32) *NormalizedTile {
	normTile := &NormalizedTile{}
	c := RGBAToBGR(transparentColor)
	normTile.Palette.Colors = append(normTile.Palette.Colors, c)
	return normTile
}

// IsTransparent reports whether the given NormalizedTile is an entirely
// transparent tile.
func (n *NormalizedTile) IsTransparent() bool {
	return len(n.Palette.Colors) == 1
}

// The Normalizer provides functionality to transform an RGBATile into a
// canonical normal form. Normalizer must be configured with an RGBA32 to treat
// as transparent.
type Normalizer struct {
	// Map sourceRGB tracks the RGBA32 that generated a given BGR15 during the normalization
	// process. This is useful because it allows the Normalizer to generate a warning diagnostic if
	// two distinct RGBA32 in the input collapsed to the same BGR15 after conversion. This usually
	// indicates some kind of user error.
	sourceRGB map[BGR15]RGBA32

	// The transparentColor is the user-configured transparency color.
	transparentColor RGBA32

	// diagEngine is a handle to the diagnostics.DiagnosticEngine for the
	// Porytiles job that is using this Normalizer.
	diagEngine *diagnostics.DiagnosticEngine
}

// NewNormalizer creates and returns a new instance of Normalizer with the given
// transparent color configuration and diagnostics.DiagnosticEngine pointer.
func NewNormalizer(transparentColor RGBA32, diagEngine *diagnostics.DiagnosticEngine) *Normalizer {
	return &Normalizer{
		sourceRGB:        make(map[BGR15]RGBA32),
		transparentColor: transparentColor,
		diagEngine:       diagEngine,
	}
}
