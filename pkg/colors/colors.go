/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

// Package colors defines the various color types that Porytiles understands.
package colors

// The Rgba32 type represents a color in RGBA32 format. The type is 4 bytes
// long: 1 byte for each color channel (red, green, blue) and 1 byte for the
// alpha channel.
type Rgba32 struct {
	Red   uint8
	Green uint8
	Blue  uint8
	Alpha uint8
}
