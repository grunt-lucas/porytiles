# Task: Add Doxygen Comments to TextFormatter System

## Overview
Add comprehensive Doxygen documentation to the TextFormatter system components and BasicError class.

## Understanding
The TextFormatter system provides a way to format text with styles (colors, bold) that adapts to TTY/non-TTY output:

- **Style**: Bitwise flags enum for text styling (bold, red, green, blue, yellow, cyan, magenta)
- **FormatParam**: Struct holding text + Style flags, used as format parameters
- **TextFormatter**: Abstract base providing `style()` and `format()` methods
- **PlainTextFormatter**: Returns text unchanged (for non-TTY/files)
- **AnsiStyledTextFormatter**: Applies ANSI escape codes (for TTY)
- **BasicError**: Error class using TextFormatter to format messages with styled parameters

The system integrates with UserDiagnostics for styled error reporting and uses fmtlib for parameter substitution.

## Todo Items

- [ ] Add Doxygen comments to Style enum and related operators/functions in text_formatter.hpp
- [ ] Add Doxygen comments to FormatParam struct in text_formatter.hpp
- [ ] Add Doxygen comments to TextFormatter class in text_formatter.hpp
- [ ] Add Doxygen comments to PlainTextFormatter class in plain_text_formatter.hpp
- [ ] Add Doxygen comments to AnsiStyledTextFormatter class in ansi_styled_text_formatter.hpp
- [ ] Enhance Doxygen comments for BasicError class in error.hpp
- [ ] Run format script to ensure consistent formatting
- [ ] Build project to verify no compilation errors
- [ ] Run tests to ensure no regressions

## Review

All Doxygen comments have been successfully added to the TextFormatter system components. Here's a summary of the changes:

### Files Modified

1. **text_formatter.hpp** - Added comprehensive documentation for:
   - Style enum with inline comments for each value
   - All bitwise operators (|, &, |=, &=)
   - has_style() helper function
   - FormatParam struct with usage example
   - TextFormatter abstract base class with detailed method documentation
   - FormattedMessageBuilder type alias

2. **plain_text_formatter.hpp** - Added documentation for:
   - PlainTextFormatter class describing its role in stripping styles
   - style() method override
   - Usage context examples

3. **ansi_styled_text_formatter.hpp** - Added documentation for:
   - AnsiStyledTextFormatter class with ANSI code details
   - Complete list of supported styles with their ANSI codes
   - style() method override with implementation notes

4. **error.hpp** - Enhanced documentation for:
   - BasicError class with comprehensive feature description
   - Both constructors (plain text and formatted)
   - details() method explaining formatter integration
   - clone() method for error chain copying
   - Replaced lengthy naming discussion comment with concise @note

### Documentation Quality

All documentation follows the project's Doxygen style guidelines:
- @brief and @details sections with blank lines between
- Parameter and return value documentation
- Usage examples with C++ code blocks
- Clear explanations of purpose and context
- Cross-references between related classes

### Testing

- Formatting: ✓ Passed (./Scripts/format.sh)
- Build: ✓ Passed (cmake --build build)
- Tests: ✓ All 159 tests passed

No regressions introduced. All code compiles cleanly and tests pass.