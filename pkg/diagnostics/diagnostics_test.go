/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

package diagnostics_test

import (
	"testing"

	"github.com/grunt-lucas/porytiles/pkg/diagnostics"
)

func TestDiagnosticEngine_Report(t *testing.T) {
	consumer := diagnostics.StderrConsumer{}
	engine := diagnostics.DiagnosticEngine{}
	engine.SetConsumer(consumer)
	messageParams := map[string]string{
		"jasc":     "255 0 255",
		"tile":     "metatile 0x1f (31), middle, northwest",
		"col":      "0",
		"row":      "1",
		"prevJasc": "251 0 251",
		"prevTile": "metatile 0x2 (2), bottom, southeast",
		"prevCol":  "4",
		"prevRow":  "2",
	}
	engine.Report(diagnostics.WarnColorPrecisionLoss, messageParams)
}
