package models_test

import (
	"fmt"
	"reflect"
	"strconv"
	"testing"

	"github.com/grunt-lucas/porytiles/pkg/models"
)

func TestRGBAToBGR(t *testing.T) {
	tests := []struct {
		name string
		rgba models.RGBA32
		want models.BGR15
	}{
		{name: "Magenta1", rgba: models.RGBA32{Red: 255, Blue: 255, Alpha: models.Opaque}, want: models.BGR15{Bgr: 0b0_11111_00000_11111}},
		{name: "Magenta2", rgba: models.RGBA32{Red: 249, Blue: 252, Alpha: models.Opaque}, want: models.BGR15{Bgr: 0b0_11111_00000_11111}},
		{name: "Red1", rgba: models.RGBA32{Red: 235, Green: 12, Blue: 81, Alpha: models.Opaque}, want: models.BGR15{Bgr: 0b0_01010_00001_11101}},
		{name: "Red2", rgba: models.RGBA32{Red: 128, Alpha: models.Transparent}, want: models.BGR15{Bgr: 0b0_00000_00000_10000}},
		{name: "Green1", rgba: models.RGBA32{Red: 3, Green: 225, Blue: 109, Alpha: models.Opaque}, want: models.BGR15{Bgr: 0b0_01101_11100_00000}},
		{name: "Green2", rgba: models.RGBA32{Red: 17, Green: 138, Alpha: models.Opaque}, want: models.BGR15{Bgr: 0b0_00000_10001_00010}},
		{name: "Blue1", rgba: models.RGBA32{Red: 90, Green: 48, Blue: 255, Alpha: models.Transparent}, want: models.BGR15{Bgr: 0b0_11111_00110_01011}},
		{name: "Blue2", rgba: models.RGBA32{Blue: 129, Alpha: models.Opaque}, want: models.BGR15{Bgr: 0b0_10000_00000_00000}},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := models.RGBAToBGR(tt.rgba); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("RGBAToBGR() = %v, want %v", got, tt.want)
			}
		})
	}
}

func ExampleRGBAToBGR() {
	rgba := models.RGBA32{Red: 235, Green: 12, Blue: 81, Alpha: models.Opaque}
	fmt.Println(strconv.FormatUint(uint64(models.RGBAToBGR(rgba).Bgr), 2))
	// Output: 10100000111101
}

func TestBGRToRGBA(t *testing.T) {
	tests := []struct {
		name string
		bgr  models.BGR15
		want models.RGBA32
	}{
		{name: "Magenta1", bgr: models.BGR15{Bgr: 0b0_11111_00000_11111}, want: models.RGBA32{Red: 248, Blue: 248, Alpha: models.Opaque}},
		{name: "Magenta2", bgr: models.BGR15{Bgr: 0b0_11110_00000_11101}, want: models.RGBA32{Red: 232, Blue: 240, Alpha: models.Opaque}},
		{name: "Red1", bgr: models.BGR15{Bgr: 0b0_01010_00001_11101}, want: models.RGBA32{Red: 232, Green: 8, Blue: 80, Alpha: models.Opaque}},
		{name: "Red2", bgr: models.BGR15{Bgr: 0b0_00000_00000_10000}, want: models.RGBA32{Red: 128, Alpha: models.Opaque}},
		{name: "Green1", bgr: models.BGR15{Bgr: 0b0_01101_11100_00000}, want: models.RGBA32{Green: 224, Blue: 104, Alpha: models.Opaque}},
		{name: "Green2", bgr: models.BGR15{Bgr: 0b0_00000_10001_00010}, want: models.RGBA32{Red: 16, Green: 136, Alpha: models.Opaque}},
		{name: "Blue1", bgr: models.BGR15{Bgr: 0b0_11111_00110_01011}, want: models.RGBA32{Red: 88, Green: 48, Blue: 248, Alpha: models.Opaque}},
		{name: "Blue2", bgr: models.BGR15{Bgr: 0b0_10000_00000_00000}, want: models.RGBA32{Blue: 128, Alpha: models.Opaque}},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := models.BGRToRGBA(tt.bgr); !reflect.DeepEqual(got, tt.want) {
				t.Errorf("BGRToRGBA() = %v, want %v", got, tt.want)
			}
		})
	}
}

func ExampleBGRToRGBA() {
	bgr := models.BGR15{Bgr: 0b0_01010_00001_11101}
	fmt.Println(models.BGRToRGBA(bgr).ToJasc())
	// Output: 232 8 80
}

func TestBGRColorChannels(t *testing.T) {
	type result struct {
		red   uint8
		green uint8
		blue  uint8
	}
	tests := []struct {
		name string
		bgr  models.BGR15
		want result
	}{
		{name: "Magenta1", bgr: models.RGBAToBGR(models.RGBA32{Red: 255, Blue: 255, Alpha: models.Opaque}), want: result{248, 0, 248}},
		{name: "Magenta2", bgr: models.RGBAToBGR(models.RGBA32{Red: 249, Blue: 252, Alpha: models.Opaque}), want: result{248, 0, 248}},
		{name: "Red1", bgr: models.RGBAToBGR(models.RGBA32{Red: 235, Green: 12, Blue: 81, Alpha: models.Opaque}), want: result{232, 8, 80}},
		{name: "Red2", bgr: models.RGBAToBGR(models.RGBA32{Red: 128, Alpha: models.Transparent}), want: result{128, 0, 0}},
		{name: "Green1", bgr: models.RGBAToBGR(models.RGBA32{Red: 3, Green: 225, Blue: 109, Alpha: models.Opaque}), want: result{0, 224, 104}},
		{name: "Green2", bgr: models.RGBAToBGR(models.RGBA32{Red: 17, Green: 138, Alpha: models.Opaque}), want: result{16, 136, 0}},
		{name: "Blue1", bgr: models.RGBAToBGR(models.RGBA32{Red: 90, Green: 48, Blue: 255, Alpha: models.Transparent}), want: result{88, 48, 248}},
		{name: "Blue2", bgr: models.RGBAToBGR(models.RGBA32{Blue: 129, Alpha: models.Opaque}), want: result{0, 0, 128}},
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

func ExampleBGR15() {
	bgr := models.BGR15{Bgr: 0b0_01010_00001_11101}
	fmt.Println(bgr.Blue(), bgr.Green(), bgr.Red())
	// Output: 80 8 232
}
