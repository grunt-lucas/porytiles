/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

package diagnostics_test

import (
	"testing"

	"github.com/grunt-lucas/porytiles/pkg/diagnostics"
)

func TestDiagnosticEngine_ReportBasic(t *testing.T) {
	consumer := &diagnostics.SliceConsumer{}
	engine := diagnostics.NewDiagnosticEngine()
	engine.SetConsumer(consumer)
	engine.Enable(diagnostics.WarnColorPrecisionLoss)
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

	expectedLen := 2
	actualLen := consumer.Len()
	if actualLen != expectedLen {
		t.Fatalf("Consumer buffer len was '%d'; expected '%d'", actualLen, expectedLen)
	}

	actualMessage := consumer.Pop()
	expectedMessage := "warning: color `255 0 255' at `metatile 0x1f (31), middle, northwest' subtile pixel col 0, row 1 collapsed to duplicate BGR"
	if actualMessage != expectedMessage {
		t.Errorf("Saw message '%s'; expected '%s'", actualMessage, expectedMessage)
	}
	actualMessage = consumer.Pop()
	expectedMessage = "note: previously saw `251 0 251' at `metatile 0x2 (2), bottom, southeast' subtile pixel col 4, row 2"
	if actualMessage != expectedMessage {
		t.Errorf("Saw message '%s'; expected '%s'", actualMessage, expectedMessage)
	}
}

func TestDiagnosticEngine_ReportWarningAsError(t *testing.T) {
	consumer := &diagnostics.SliceConsumer{}
	engine := diagnostics.NewDiagnosticEngine()
	engine.SetConsumer(consumer)
	engine.Enable(diagnostics.WarnColorPrecisionLoss)
	engine.OverrideLevel(diagnostics.WarnColorPrecisionLoss, diagnostics.Error)
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

	expectedLen := 2
	actualLen := consumer.Len()
	if actualLen != expectedLen {
		t.Fatalf("Consumer buffer len was '%d'; expected '%d'", actualLen, expectedLen)
	}

	actualMessage := consumer.Pop()
	expectedMessage := "error: color `255 0 255' at `metatile 0x1f (31), middle, northwest' subtile pixel col 0, row 1 collapsed to duplicate BGR"
	if actualMessage != expectedMessage {
		t.Errorf("Saw message '%s'; expected '%s'", actualMessage, expectedMessage)
	}
	// We don't care about the note text for this test
}

func TestDiagnosticEngine_ReportAllWarningsEnabled(t *testing.T) {
	consumer := &diagnostics.SliceConsumer{}
	engine := diagnostics.NewDiagnosticEngine()
	engine.SetConsumer(consumer)
	engine.EnableAllWarnings()
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

	expectedLen := 2
	actualLen := consumer.Len()
	if actualLen != expectedLen {
		t.Fatalf("Consumer buffer len was '%d'; expected '%d'", actualLen, expectedLen)
	}
}
