package colors_test

import (
	"reflect"
	"testing"

	"github.com/grunt-lucas/porytiles/pkg/colors"
)

func TestRGBAToBGR(t *testing.T) {
	tests := []struct {
		name string
		rgba colors.RGBA32
		want colors.BGR15
	}{
		{name: "Magenta1", rgba: colors.RGBA32{255, 0, 255, colors.Opaque}, want: colors.BGR15{0b0_11111_00000_11111}},
		{name: "Magenta2", rgba: colors.RGBA32{249, 0, 252, colors.Opaque}, want: colors.BGR15{0b0_11111_00000_11111}},
		{name: "Red1", rgba: colors.RGBA32{235, 12, 81, colors.Opaque}, want: colors.BGR15{0b0_01010_00001_11101}},
		{name: "Red2", rgba: colors.RGBA32{128, 0, 0, colors.Transparent}, want: colors.BGR15{0b0_00000_00000_10000}},
		{name: "Green1", rgba: colors.RGBA32{3, 225, 109, colors.Opaque}, want: colors.BGR15{0b0_01101_11100_00000}},
		{name: "Green2", rgba: colors.RGBA32{17, 138, 0, colors.Opaque}, want: colors.BGR15{0b0_00000_10001_00010}},
		{name: "Blue1", rgba: colors.RGBA32{90, 48, 255, colors.Transparent}, want: colors.BGR15{0b0_11111_00110_01011}},
		{name: "Blue2", rgba: colors.RGBA32{0, 0, 129, colors.Opaque}, want: colors.BGR15{0b0_10000_00000_00000}},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := colors.RGBAToBGR(tt.rgba); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("RGBAToBGR() = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestBGRToRGBA(t *testing.T) {
	tests := []struct {
		name string
		bgr  colors.BGR15
		want colors.RGBA32
	}{
		{name: "Magenta1", bgr: colors.BGR15{0b0_11111_00000_11111}, want: colors.RGBA32{248, 0, 248, colors.Opaque}},
		{name: "Magenta2", bgr: colors.BGR15{0b0_11110_00000_11101}, want: colors.RGBA32{232, 0, 240, colors.Opaque}},
		{name: "Red1", bgr: colors.BGR15{0b0_01010_00001_11101}, want: colors.RGBA32{232, 8, 80, colors.Opaque}},
		{name: "Red2", bgr: colors.BGR15{0b0_00000_00000_10000}, want: colors.RGBA32{128, 0, 0, colors.Opaque}},
		{name: "Green1", bgr: colors.BGR15{0b0_01101_11100_00000}, want: colors.RGBA32{0, 224, 104, colors.Opaque}},
		{name: "Green2", bgr: colors.BGR15{0b0_00000_10001_00010}, want: colors.RGBA32{16, 136, 0, colors.Opaque}},
		{name: "Blue1", bgr: colors.BGR15{0b0_11111_00110_01011}, want: colors.RGBA32{88, 48, 248, colors.Opaque}},
		{name: "Blue2", bgr: colors.BGR15{0b0_10000_00000_00000}, want: colors.RGBA32{0, 0, 128, colors.Opaque}},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := colors.BGRToRGBA(tt.bgr); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("BGRToRGBA() = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestBGRColorChannels(t *testing.T) {
	type result struct {
		red   uint8
		green uint8
		blue  uint8
	}
	tests := []struct {
		name string
		bgr  colors.BGR15
		want result
	}{
		{name: "Magenta1", bgr: colors.RGBAToBGR(colors.RGBA32{255, 0, 255, colors.Opaque}), want: result{248, 0, 248}},
		{name: "Magenta2", bgr: colors.RGBAToBGR(colors.RGBA32{249, 0, 252, colors.Opaque}), want: result{248, 0, 248}},
		{name: "Red1", bgr: colors.RGBAToBGR(colors.RGBA32{235, 12, 81, colors.Opaque}), want: result{232, 8, 80}},
		{name: "Red2", bgr: colors.RGBAToBGR(colors.RGBA32{128, 0, 0, colors.Transparent}), want: result{128, 0, 0}},
		{name: "Green1", bgr: colors.RGBAToBGR(colors.RGBA32{3, 225, 109, colors.Opaque}), want: result{0, 224, 104}},
		{name: "Green2", bgr: colors.RGBAToBGR(colors.RGBA32{17, 138, 0, colors.Opaque}), want: result{16, 136, 0}},
		{name: "Blue1", bgr: colors.RGBAToBGR(colors.RGBA32{90, 48, 255, colors.Transparent}), want: result{88, 48, 248}},
		{name: "Blue2", bgr: colors.RGBAToBGR(colors.RGBA32{0, 0, 129, colors.Opaque}), want: result{0, 0, 128}},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := tt.bgr.Red(); !reflect.DeepEqual(got, tt.want.red) {
				t.Errorf("Red() = %v, want %v", got, tt.want.red)
			}
			if got := tt.bgr.Green(); !reflect.DeepEqual(got, tt.want.green) {
				t.Errorf("Green() = %v, want %v", got, tt.want.green)
			}
			if got := tt.bgr.Blue(); !reflect.DeepEqual(got, tt.want.blue) {
				t.Errorf("Blue() = %v, want %v", got, tt.want.blue)
			}
		})
	}
}
