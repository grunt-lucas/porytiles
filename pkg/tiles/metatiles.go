/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

package tiles

type Attributes struct {
}

type MetatileEntry struct {
	TileIndex int
	PalIndex  int
	HFlip     bool
	VFlip     bool
}

type Metatile struct {
	Subtiles []MetatileEntry
	Attr     Attributes
}
