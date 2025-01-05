package config_test

import (
	"fmt"
	"testing"

	"github.com/grunt-lucas/porytiles/pkg/config"
)

func TestFieldmapConfig_NumPalettesInSecondary(t *testing.T) {
	config := config.FieldmapConfig{512, 1024, 512, 1024, 6, 13, 12}
	expected := 7
	actual := config.NumPalettesInSecondary()
	if actual != expected {
		t.Errorf("NumPalettesInSecondary() = %d; expected %d", actual, expected)
	}
}

func ExampleFieldmapConfig_NumPalettesInSecondary() {
	config := config.FieldmapConfig{512, 1024, 512, 1024, 6, 13, 12}
	fmt.Println(config.NumPalettesInSecondary())
	// Output: 7
}
