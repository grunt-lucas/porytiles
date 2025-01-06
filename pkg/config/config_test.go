package config_test

import (
	"fmt"
	"testing"

	"github.com/grunt-lucas/porytiles/pkg/config"
)

func TestFieldmapConfig_NumPalettesInSecondary(t *testing.T) {
	fieldmapConfig := config.FieldmapConfig{}
	fieldmapConfig.NumPalettesInPrimary = 6
	fieldmapConfig.NumPalettesTotal = 13
	expected := 7
	actual := fieldmapConfig.NumPalettesInSecondary()
	if actual != expected {
		t.Errorf("NumPalettesInSecondary() = %d; expected %d", actual, expected)
	}
}

func ExampleFieldmapConfig_NumPalettesInSecondary() {
	fieldmapConfig := config.FieldmapConfig{}
	fieldmapConfig.NumPalettesInPrimary = 6
	fieldmapConfig.NumPalettesTotal = 13
	fmt.Println(fieldmapConfig.NumPalettesInSecondary())
	// Output: 7
}
