/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

package models

type PorytilesAnimFrame struct {
	Tiles []RGBATile
	Name  string
}

// Size returns the size of the Tiles slice for this PorytilesAnimFrame.
func (p *PorytilesAnimFrame) Size() int {
	return len(p.Tiles)
}

type PorytilesAnim struct {
	Key    []RGBATile
	Frames []PorytilesAnimFrame
	Name   string
}

type PorymapAnimFrame struct {
	Tiles []GBATile
	Name  string
}

type PorymapAnim struct {
	Key    []GBATile
	Frames []PorymapAnimFrame
	Name   string
}
