/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

package models

// A PorytilesTileset is a tileset in Porytiles format.
type PorytilesTileset struct {
	Tiles       []RGBATile
	TripleLayer bool
}

// A PorymapTileset is a tileset in Porymap format.
type PorymapTileset struct {
	Tiles          []GBATile
	Metatiles      []Metatile
	PalIndexOfTile []int
	Pals           []GBAPalette
	ColorIndexMap  map[BGR15]int
	TileIndexMap   map[GBATile]int
}
