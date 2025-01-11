/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

// Package diagnostics defines the user diagnostics system for Porytiles jobs.
package diagnostics

import (
	"os"
	"strconv"
	"strings"
	"text/template"

	"github.com/fatih/color"
)

// ThrowInternalPanic panics with the given message to signal an internal
// Porytiles error. Typically, this error is reserved for bad internal state
// (which usually signals the presence of a bug). End users should not see this
// error upon bad input or other correctable problems.
func ThrowInternalPanic(msg string) {
	yellow := color.New(color.FgYellow, color.Bold)
	yellow.Fprintf(os.Stderr, "internal porytiles error: ")
	panic(msg)
}

// DiagnosticLevel represents a particular severity level for a Diagnostic. The
// severity levels are similar to those present in other popular compilation
// toolchains like Clang (i.e. Note, Warning, Error, etc).
type DiagnosticLevel int

const (
	// Ignored means ignore this Diagnostic entirely. This could be because the
	// user explicitly requested to ignore via '-Wno-this-diag', or due to
	// internal job state, etc.
	Ignored DiagnosticLevel = iota

	// The Note level is intended only for a Diagnostic that is linked to a
	// previous Diagnostic of a higher severity level.
	Note

	// The Remark level is for a Diagnostic which alerts the user to some
	// relevant action a given Porytiles job has taken, e.g. some optimization,
	// on-the-fly edit of the input data, etc.
	Remark

	// The Warning level is for a Diagnostic which alerts the user to a
	// potential problem with their input or job settings. However, warnings
	// will not terminate the job.
	Warning

	// The Error level is for a Diagnostic which is not recoverable, but for
	// which the Porytiles job can continue for a while in order to possibly
	// generate additional useful Diagnostics.
	Error

	// The Fatal level is for a Diagnostic which demands immediate termination
	// of the Porytiles job.
	Fatal
	FOOBAR
)

// DiagnosticTemplate defines a reusable template for standardized diagnostic
// reporting. The DiagnosticEngine uses a template to construct the actual
// Diagnostic instance when one is in-flight.
type DiagnosticTemplate struct {
	name         string
	defaultLevel DiagnosticLevel
	message      string
}

func formatMessage(diag *DiagnosticTemplate, m map[string]string) string {
	templ := template.Must(template.New("").Parse(diag.message))
	result := &strings.Builder{}
	err := templ.Execute(result, m)
	if err != nil {
		ThrowInternalPanic("failed to format diagnostic message: " + err.Error())
	}
	return result.String()
}

var WarnColorPrecisionLossTemplate = DiagnosticTemplate{
	name:         "color-precision-loss",
	defaultLevel: Warning,
	message:      "color `{{.jasc}}' at {{.mode}} `{{.tile}}' subtile pixel col {{.col}}, row {{.row}} collapsed to duplicate BGR",
}

type Diagnostic struct {
	id       int
	template *DiagnosticTemplate
	level    DiagnosticLevel
	message  string
}

type DiagnosticConsumer interface {
	ConsumeDiagnostic(diag *Diagnostic)
}

type StderrConsumer struct{}

func (s StderrConsumer) ConsumeDiagnostic(diag *Diagnostic) {
	// TODO : implement me
	switch diag.level {
	case Ignored:
		// Do nothing for ignored diagnostics
	case Note:
		// Process Note level diagnostics
	case Remark:
		// Process Remark level diagnostics
	case Warning:
		// Process Warning level diagnostics
	case Error:
		// Process Error level diagnostics
	case Fatal:
		// Process Fatal level diagnostics
		os.Exit(1)
	default:
		// Handle unexpected diagnostic levels
		ThrowInternalPanic("unexpected Diagnostic level: " + strconv.Itoa(int(diag.level)))
	}
}

type IgnoreConsumer struct{}

func (s IgnoreConsumer) ConsumeDiagnostic(diag *Diagnostic) {
	// explicitly do nothing
}

type DiagnosticEngine struct {
	consumer DiagnosticConsumer
}
