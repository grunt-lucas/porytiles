package config

import (
	"fmt"
	"testing"
)

func TestFieldmapConfig_NumPalettesInSecondary(t *testing.T) {
	config := FieldmapConfig{512, 1024, 512, 1024, 6, 13, 12}
	if config.NumPalettesInSecondary() != 7 {
		t.Fatal("failed")
	}
}

func ExampleFieldmapConfig_NumPalettesInSecondary() {
	config := FieldmapConfig{512, 1024, 512, 1024, 6, 13, 12}
	fmt.Println(config.NumPalettesInSecondary())
	// Output: 7
}
