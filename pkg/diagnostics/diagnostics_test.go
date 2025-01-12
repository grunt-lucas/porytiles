package diagnostics_test

import (
	"testing"

	"github.com/grunt-lucas/porytiles/pkg/diagnostics"
)

func TestDiagnosticEngine_Report(t *testing.T) {
	consumer := diagnostics.StderrConsumer{}
	engine := diagnostics.DiagnosticEngine{}
	engine.SetConsumer(consumer)
	engine.Report(diagnostics.WarnColorPrecisionLossTemplate, map[string]string{})
}
