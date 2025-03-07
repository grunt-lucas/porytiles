/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

package models

type LayerType int

const (
	LayerNormal LayerType = iota
	LayerCovered
	LayerSplit
	LayerTriple
)

type EncounterType int

const (
	EncounterNone EncounterType = iota
	EncounterLand
	EncounterWater
)

type TerrainType int

const (
	TerrainNormal TerrainType = iota
	TerrainGrass
	TerrainWater
	TerrainWaterfall
)

type Attributes struct {
	LayerType     LayerType
	EncounterType EncounterType
	TerrainType   TerrainType
	Behavior      uint16
}
