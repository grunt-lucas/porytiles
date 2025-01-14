/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

// Package config defines the various configuration types needed to configure a Porytiles job.
package config

// A TilesOutputPalette represents a possible output format for Porymap-format tiles (i.e. the
// canonical 'tiles.png' file). Vanilla tileset tiles use Greyscale, but Porymap now supports tiles
// in TrueColor format.
type TilesOutputPalette int

const (
	// A TrueColor 'tiles.png' contains the full palette configuration in the internal PNG palette.
	// The bottom four bits of each pixel will correctly index into an individual palette, so
	// 'gbagfx' will be perfectly happy. However, the top 4 bits will index into one of the internal
	// PNG palettes. This means that the 'tiles.png' will be realistically colored, so it will be
	// easy to see what a tile will actually look like in-game.
	TrueColor TilesOutputPalette = iota

	// A Greyscale 'tiles.png' contains a single 16-color greyscale palette. This makes it easy to
	// see the index value of each pixel. But the tradeoff is that it's not obvious which actual
	// palette (i.e. which color) each pixel is actually using.
	Greyscale
)

// A TargetBaseGame represents which of the three Generation III decompilation projects this
// Porytiles job is targeting. Setting the TargetBaseGame is a more convenient way to set the
// fieldmap parameters for a Porytiles job. Think of it like a preset for the FieldmapConfig.
type TargetBaseGame int

const (
	Emerald TargetBaseGame = iota
	FireRed
	Ruby
)

// FieldmapConfig is Porytiles's internal representation of the fieldmap defines at the very top of
// 'include/fieldmap.h'.
type FieldmapConfig struct {
	NumTilesInPrimary     int
	NumTilesTotal         int
	NumMetatilesInPrimary int
	NumMetatilesTotal     int
	NumPalettesInPrimary  int
	NumPalettesTotal      int
	NumTilesPerMetatile   int
}

// EmeraldDefaults creates a FieldmapConfig with corresponding to Emerald's 'include/fieldmap.h'
// configuration.
func EmeraldDefaults() FieldmapConfig {
	fieldmapConfig := FieldmapConfig{}
	fieldmapConfig.NumTilesInPrimary = 512
	fieldmapConfig.NumTilesTotal = 1024
	fieldmapConfig.NumMetatilesInPrimary = 512
	fieldmapConfig.NumMetatilesTotal = 1024
	fieldmapConfig.NumPalettesInPrimary = 6
	fieldmapConfig.NumPalettesTotal = 13
	fieldmapConfig.NumTilesPerMetatile = 12
	return fieldmapConfig
}

// FireRedDefaults creates a FieldmapConfig with corresponding to FireRed's 'include/fieldmap.h'
// configuration.
func FireRedDefaults() FieldmapConfig {
	fieldmapConfig := FieldmapConfig{}
	fieldmapConfig.NumTilesInPrimary = 640
	fieldmapConfig.NumTilesTotal = 1024
	fieldmapConfig.NumMetatilesInPrimary = 640
	fieldmapConfig.NumMetatilesTotal = 1024
	fieldmapConfig.NumPalettesInPrimary = 7
	fieldmapConfig.NumPalettesTotal = 13
	fieldmapConfig.NumTilesPerMetatile = 12
	return fieldmapConfig
}

// RubyDefaults creates a FieldmapConfig with corresponding to Ruby's 'include/fieldmap.h'
// configuration.
func RubyDefaults() FieldmapConfig {
	fieldmapConfig := FieldmapConfig{}
	fieldmapConfig.NumTilesInPrimary = 512
	fieldmapConfig.NumTilesTotal = 1024
	fieldmapConfig.NumMetatilesInPrimary = 512
	fieldmapConfig.NumMetatilesTotal = 1024
	fieldmapConfig.NumPalettesInPrimary = 6
	fieldmapConfig.NumPalettesTotal = 12
	fieldmapConfig.NumTilesPerMetatile = 12
	return fieldmapConfig
}

// The GlobalConfig stores configuration data that can apply to a Porytiles job of any type.
// Job-type-specific configuration data is defined separately in the corresponding config structs.
type GlobalConfig struct {
	TargetBaseGame TargetBaseGame
	FieldmapConfig FieldmapConfig
}

// NumPalettesInSecondary returns the number of secondary tileset palettes that are implied by the
// given [FieldmapConfig] config.
func (config *FieldmapConfig) NumPalettesInSecondary() int {
	return config.NumPalettesTotal - config.NumPalettesInPrimary
}

// NumTilesInSecondary returns the number of secondary tileset tiles that are implied by the given
// [FieldmapConfig] config.
func (config *FieldmapConfig) NumTilesInSecondary() int {
	return config.NumTilesTotal - config.NumTilesInPrimary
}

// NumMetatilesInSecondary returns the number of secondary tileset metatiles that are implied by the
// given [FieldmapConfig] config.
func (config *FieldmapConfig) NumMetatilesInSecondary() int {
	return config.NumMetatilesTotal - config.NumMetatilesInPrimary
}
