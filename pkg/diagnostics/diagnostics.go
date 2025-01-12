/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

// Package diagnostics defines the user diagnostics system for Porytiles jobs.
package diagnostics

import (
	"fmt"
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
	yellow := color.New(color.FgYellow, color.Bold).SprintFunc()
	fmt.Fprintf(os.Stderr, "%s %s\n", yellow("internal porytiles error:"), msg)
	panic("internal error")
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
	Name         string
	DefaultLevel DiagnosticLevel
	Message      string
	Note         string
}

const WarnColorPrecisionLoss = "color-precision-loss"

var WarnColorPrecisionLossTemplate = DiagnosticTemplate{
	Name:         WarnColorPrecisionLoss,
	DefaultLevel: Warning,
	Message:      "color `{{.jasc}}' at {{.mode}} `{{.tile}}' subtile pixel col {{.col}}, row {{.row}} collapsed to duplicate BGR",
	Note:         "previously saw `{{.jasc}}' at `{{.tile}}' subtile pixel col {{.col}}, row {{.row}}",
}

type Diagnostic struct {
	// id       int
	template DiagnosticTemplate
	level    DiagnosticLevel
	message  string
}

type DiagnosticConsumer interface {
	ConsumeDiagnostic(diag Diagnostic)
}

type StderrConsumer struct{}

func (s StderrConsumer) ConsumeDiagnostic(diag Diagnostic) {
	// TODO : implement me
	switch diag.level {
	case Ignored:
		// Do nothing for ignored diagnostics
	case Note:
		// Process Note level diagnostics
	case Remark:
		// Process Remark level diagnostics
	case Warning:
		magenta := color.New(color.FgMagenta, color.Bold).SprintFunc()
		fmt.Fprintf(os.Stderr, "%s %s\n", magenta("warning:"), diag.message)
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

type DevNullConsumer struct{}

func (s DevNullConsumer) ConsumeDiagnostic(diag Diagnostic) {
	// explicitly do nothing
}

type DiagnosticEngine struct {
	consumer DiagnosticConsumer
}

func (engine *DiagnosticEngine) SetConsumer(consumer DiagnosticConsumer) {
	engine.consumer = consumer
}

func (engine *DiagnosticEngine) Report(diagTempl DiagnosticTemplate, m map[string]string) {
	// TODO : is there a better way to handle this than templates?
	templ := template.Must(template.New("").Parse(diagTempl.Message))
	formattedMessage := &strings.Builder{}
	err := templ.Execute(formattedMessage, m)
	if err != nil {
		ThrowInternalPanic("failed to format diagnostic message: " + err.Error())
	}
	// TODO : look up actual level in a map that DiagnosticEngine manages (to handle user-modified levels)
	diagnostic := Diagnostic{diagTempl, diagTempl.DefaultLevel, formattedMessage.String()}
	engine.consumer.ConsumeDiagnostic(diagnostic)
}
