/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

package models

import "strconv"

// A RGBA32 is a color in RGBA32 format. The type is 4 bytes long: 1 byte for
// each color channel (red, green, blue) and 1 byte for the alpha channel.
type RGBA32 struct {
	Red   uint8
	Green uint8
	Blue  uint8
	Alpha uint8
}

func (rgba RGBA32) ToJasc() string {
	redString := strconv.Itoa(int(rgba.Red))
	greenString := strconv.Itoa(int(rgba.Green))
	blueString := strconv.Itoa(int(rgba.Blue))
	return redString + " " + greenString + " " + blueString
}

// Transparent represents an 8-bit alpha value of 0, indicating full
// transparency for a Rgba32 Alpha channel
const Transparent uint8 = 0

// Opaque represents an 8-bit alpha value of 255, indicating full opacity for a
// Rgba32 Alpha channel.
const Opaque uint8 = 255

// A BGR15 is a color in BGR15 format. Each color channel has five bits of
// precision. GBA colors are represented in BGR15. Implementation requires that
// the most significant bit go unused, followed by five bits for blue, then
// green, then red, respectively.
type BGR15 struct {
	Bgr uint16
}

// Blue returns the blue channel value of the given BGR15, scaled up to an 8-bit
// value.
func (bgr BGR15) Blue() uint8 {
	return uint8(((bgr.Bgr >> 10) & 0x001f) << 3)
}

// Green returns the green channel value of the given BGR15, scaled up to an
// 8-bit value.
func (bgr BGR15) Green() uint8 {
	return uint8(((bgr.Bgr >> 5) & 0x001f) << 3)
}

// Red returns the red channel value of the given BGR15, scaled up to an 8-bit
// value.
func (bgr BGR15) Red() uint8 {
	return uint8(((bgr.Bgr) & 0x001f) << 3)
}

// RGBAToBGR converts a color from RGBA32 format to BGR15 format.
func RGBAToBGR(rgba RGBA32) BGR15 {
	// Extract the 5 most significant bits of each color channel
	red := uint16((rgba.Red >> 3) & 0x1f)
	green := uint16((rgba.Green >> 3) & 0x1f)
	blue := uint16((rgba.Blue >> 3) & 0x1f)

	// Pack the values into a uint16 in the format:
	// unused(1 bit) | blue(5 bits) | green(5 bits) | red(5 bits)
	bgrValue := blue<<10 | green<<5 | red

	return BGR15{Bgr: bgrValue}
}

// BGRToRGBA converts a color from BGR15 format to RGBA32 format. The converted
// RGBA32 will always have an alpha channel set to full opacity.
func BGRToRGBA(bgr BGR15) RGBA32 {
	// Extract each 5-bit color channel, then scale back up to 8-bits
	red := uint8(bgr.Bgr&0x1f) << 3
	green := uint8((bgr.Bgr>>5)&0x1f) << 3
	blue := uint8((bgr.Bgr>>10)&0x1f) << 3
	alpha := Opaque

	return RGBA32{Red: red, Green: green, Blue: blue, Alpha: alpha}
}
