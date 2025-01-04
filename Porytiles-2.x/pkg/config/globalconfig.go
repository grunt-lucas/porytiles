/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

// Package config defines the various types needed to configure a Porytiles job.
package config

// A TargetBaseGame represents which of the three Generation III decompilation
// projects this Porytiles job is targeting. Setting the TargetBaseGame is a
// more convenient way to set the fieldmap parameters for the job. Think of it
// like a preset for the fieldmap configuration.
type TargetBaseGame int

const (
	Emerald TargetBaseGame = iota // foo
	FireRed
	Ruby
)

// The FieldmapConfig fields represent the fieldmap defines at the very top of
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

// The GlobalConfig stores configuration data that can apply to a Porytiles job
// of any type. Job-type-specific configuration data is defined separately in
// the corresponding config structs.
type GlobalConfig struct {
	TargetBaseGame TargetBaseGame
	FieldmapConfig FieldmapConfig
}

// NumPalettesInSecondary returns the number of secondary tileset palettes that
// are implied by the given [FieldmapConfig] config.
func (config *FieldmapConfig) NumPalettesInSecondary() int {
	return config.NumPalettesTotal - config.NumPalettesInPrimary
}

// NumTilesInSecondary returns the number of secondary tileset tiles that are
// implied by the given [FieldmapConfig] config.
func (config *FieldmapConfig) NumTilesInSecondary() int {
	return config.NumTilesTotal - config.NumTilesInPrimary
}

// NumMetatilesInSecondary returns the number of secondary tileset metatiles
// that are implied by the given [FieldmapConfig] config.
func (config *FieldmapConfig) NumMetatilesInSecondary() int {
	return config.NumMetatilesTotal - config.NumMetatilesInPrimary
}
