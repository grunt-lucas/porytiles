/*
Copyright © 2025 grunt-lucas grunt.lucas@yahoo.com
*/

// Package diagnostics provides the user diagnostics system for Porytiles jobs.
package diagnostics

import (
	"fmt"
	"os"

	"github.com/fatih/color"
)

// PorytilesInternalPanic panics with the given message to signal an internal Porytiles error.
// Typically, this error is reserved for bad internal state (which usually signals the presence of a
// bug). End users should not see this error upon bad input or other correctable problems.
func PorytilesInternalPanic(msg string) {
	yellow := color.New(color.FgYellow, color.Bold).SprintFunc()
	fmt.Fprintf(os.Stderr, "%s %s\n", yellow("internal porytiles error:"), msg)
	panic("internal error")
}

// DiagnosticLevel represents a particular severity level for a diagnostic. The severity levels are
// similar to those present in other popular compilation toolchains like Clang (i.e. Note, Warning,
// Error, etc).
type DiagnosticLevel string

const (
	// Ignored means ignore this diagnostic entirely. This could be because the user explicitly
	// requested to ignore via '-Wno-this-diag', or due to internal job state, etc.
	Ignored DiagnosticLevel = "ignored"

	// The Note level is intended only for a diagnostic that is linked to a previous diagnostic of a
	// higher severity level. Diagnostics can define zero or more notes that the DiagnosticEngine
	// should emit alongside the main diagnostic message. Notes provide additional context to help
	// users resolve issues.
	Note DiagnosticLevel = "note"

	// The Remark level is for a diagnostic which alerts the user to some relevant action a given
	// Porytiles job has taken, e.g. some optimization, on-the-fly edit of the input data, etc.
	// Unlike Note, Remark can be freestanding.
	Remark DiagnosticLevel = "remark"

	// The Warning level is for a diagnostic which alerts the user to a potential problem with their
	// input or job settings. However, warnings will not terminate the job.
	Warning DiagnosticLevel = "warning"

	// The Error level is for a diagnostic which is not recoverable, but for which the Porytiles job
	// can continue for a while in order to possibly generate additional useful Diagnostics.
	Error DiagnosticLevel = "error"

	// The Fatal level is for a diagnostic which demands immediate termination of the Porytiles job.
	Fatal DiagnosticLevel = "fatal"
)

// DiagnosticTemplate defines a reusable template for standardized diagnostic reporting. The
// DiagnosticEngine uses a template to construct the actual diagnostic instance when one is
// in-flight. A DiagnosticTemplate defines a unique name for the diagnostic as well as some default
// settings. Additionally, it provides a template for the diagnostic message and templates for zero
// or more notes that can be emitted alongside the diagnostic.
type DiagnosticTemplate struct {
	Name            string
	DefaultEnabled  bool
	DefaultLevel    DiagnosticLevel
	MessageTemplate string
	NoteTemplates   []string
}

// WarnColorPrecisionLoss ("color-precision-loss") warns when two input colors will collapse to the
// same color upon BGR conversion.
//
// See: <https://github.com/grunt-lucas/porytiles/wiki/Warnings-and-Errors#-wcolor-precision-loss>
const WarnColorPrecisionLoss = "color-precision-loss"

// WarnColorPrecisionLossMessage is the warning message shown by Porytiles when a
// WarnColorPrecisionLoss is in-flight.
//
// MessageTemplate Parameters
//
//	{{.jasc}}       - the JASC PAL file representation of the relevant color
//	{{.tile}}       - the relevant tile's string representation
//	{{.col}}        - the offending pixel's column, indexed within the subtile
//	{{.row}}        - the offending pixel's row, indexed within the subtile
const WarnColorPrecisionLossMessage = "color `{{.jasc}}' at `{{.tile}}' subtile pixel col {{.col}}, row {{.row}} collapsed to duplicate BGR"

// WarnColorPrecisionLossNotes are the supplemental notes Porytiles will display alongside the
// message for a WarnColorPrecisionLoss.
//
// MessageTemplate Parameters
//
//	{{.prevJasc}}       - the JASC PAL file representation of the colliding color
//	{{.prevTile}}       - the previous tile that contained the colliding color
//	{{.prevCol}}        - the previous pixel's column, indexed within the subtile
//	{{.prevRow}}        - the previous pixel's row, indexed within the subtile
var WarnColorPrecisionLossNotes = []string{
	"previously saw `{{.prevJasc}}' at `{{.prevTile}}' subtile pixel col {{.prevCol}}, row {{.prevRow}}",
}

var warnColorPrecisionLossTemplate = DiagnosticTemplate{
	Name:            WarnColorPrecisionLoss,
	DefaultEnabled:  false,
	DefaultLevel:    Warning,
	MessageTemplate: WarnColorPrecisionLossMessage,
	NoteTemplates:   WarnColorPrecisionLossNotes,
}

// Map of unique diagnostic names to their templates
var diagNameToTemplate = map[string]DiagnosticTemplate{
	WarnColorPrecisionLoss: warnColorPrecisionLossTemplate,
}

func getDiagnosticTemplate(diagName string) DiagnosticTemplate {
	diagTempl, ok := diagNameToTemplate[diagName]
	if !ok {
		PorytilesInternalPanic("diagnostic with name `" + diagName + "' not found")
	}
	return diagTempl
}
