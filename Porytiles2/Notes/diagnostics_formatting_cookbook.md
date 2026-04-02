# Diagnostics & Formatting Cookbook

Quick-reference recipes for the Porytiles2 diagnostic and formatting APIs, drawn from real
codebase usage. Each section covers one API surface with copy-pasteable patterns.

**Key headers:**

```c++
#include "porytiles2/utilities/text/text_formatter.hpp"       // Style, FormatParam, TextFormatter
#include "porytiles2/utilities/result/error.hpp"              // Error, FormattableError
#include "porytiles2/utilities/result/chainable_result.hpp"   // ChainableResult, PT_TRY_* macros
#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"   // UserDiagnostics
#include "porytiles2/domain/algorithms/diagnostic_stencils.hpp" // Stencil helpers
```

---

## 1. Style & FormatParam Quick Reference

### Style Constants

| Category       | Constants                                                                     |
|----------------|-------------------------------------------------------------------------------|
| **Format**     | `Style::none`, `Style::bold`, `Style::faint`, `Style::italic`, `Style::underline`, `Style::blink` |
| **Foreground** | `Style::black`, `Style::red`, `Style::green`, `Style::yellow`, `Style::blue`, `Style::magenta`, `Style::cyan`, `Style::white` |
| **Background** | `Style::bg_black`, `Style::bg_red`, `Style::bg_green`, `Style::bg_yellow`, `Style::bg_blue`, `Style::bg_magenta`, `Style::bg_cyan`, `Style::bg_white` |
| **Custom RGB** | `rgb_fg_style(r, g, b)`, `rgb_bg_style(r, g, b)` |

### Combining Styles

Combine with `operator|`. The right-hand side wins for color conflicts.

```c++
Style::bold | Style::red                      // Bold red foreground
Style::bold | Style::yellow                   // Bold yellow foreground
Style::red | Style::bg_blue                   // Red text on blue background
rgb_fg_style(255, 128, 0) | Style::bg_black   // Custom orange on black
```

### FormatParam Constructors

```c++
// 1. Unstyled string (implicit conversion)
FormatParam{name}

// 2. Styled string
FormatParam{name, Style::bold}

// 3. Unstyled non-string formattable value (implicit conversion via std::format)
FormatParam{42}
FormatParam{metatiles.size()}

// 4. Styled non-string formattable value
FormatParam{metatiles.size(), Style::bold}
FormatParam{pal::max_size - 1, Style::bold | Style::yellow}
```

**Rule:** Any type satisfying `std::formattable<T, char>` can be passed directly.
Custom types need a `std::formatter<T>` specialization that delegates to `porytiles2::to_string()`.

---

## 2. TextFormatter Recipes

Access the formatter via `UserDiagnostics::formatter()` or through a stored reference.

### `style()` — Apply a Style to a Raw String

```c++
std::string styled = formatter.style("some text", Style::bold | Style::red);
```

### `format()` — Variadic FormatParams

```c++
// Single parameter
std::string msg = formatter.format(
    "Detected base game '{}'.",
    FormatParam{to_string(detected), Style::bold});

// Multiple parameters
std::string msg = formatter.format(
    "{} * {} = {}",
    FormatParam{num_pals.value(), Style::bold | Style::yellow},
    FormatParam{pal::max_size - 1, Style::bold},
    FormatParam{color_count_limit, Style::bold});
```

> _From `base_game_detector.cpp` and `diagnostic_stencils.hpp`._

### `format()` — Vector Overload

```c++
std::vector<FormatParam> params;
params.emplace_back(FormatParam{filename, Style::bold});
params.emplace_back(FormatParam{line_number});
std::string msg = formatter.format("Error at '{}' line {}", params);
```

### `format()` for Building Lines in a Vector

A common pattern: call `format()` inline while pushing into a `std::vector<std::string>`:

```c++
std::vector<std::string> lines;
lines.push_back(formatter.format(
    "Porymap palette '{}': {}:",
    FormatParam{pal_label, Style::bold},
    FormatParam{message}));
```

> _From `diagnostic_stencils.hpp:136`._

---

## 3. FormattableError Recipes

FormattableError has **six construction patterns**, from simplest to most complex.

### Pattern A: Empty (Passthrough)

Used when the current layer adds no context—just propagates an existing error chain.

```c++
FormattableError{}
```

Typically used inside `PT_TRY_ASSIGN_PASS_ERR` or `PT_TRY_CALL_PASS_ERR` (see §7).

### Pattern B: Plain String

```c++
FormattableError{"Tileset begin transaction failed."}
FormattableError{"Failed to match all Porytiles tiles."}
```

> _From `tileset_repo.cpp:20` and `primary_tileset_compiler.cpp:485`._

### Pattern C: Format String + Variadic FormatParams

```c++
FormattableError{
    "Tileset '{}' does not exist.",
    FormatParam{tileset_name, Style::bold}}

FormattableError{
    "Found '{}' metatiles, limit is '{}'.",
    FormatParam{metatiles.size(), Style::bold},
    FormatParam{metatile_limit, Style::bold}}
```

> _From `compile_primary_tileset.cpp:18` and `tileset_compile_validators.hpp:240`._

### Pattern D: Format String + Vector of FormatParams

```c++
FormattableError{
    "Expected {} but got {}",
    std::vector<FormatParam>{
        FormatParam{expected, Style::green},
        FormatParam{actual, Style::red}}}
```

### Pattern E: Vector of Strings (Multi-line, No Per-line Params)

Build up a `std::vector<std::string>`, formatting each line yourself via the formatter, then
pass the whole vector.

```c++
std::vector<std::string> err_msg{};
err_msg.emplace_back(diag_->formatter().format(
    "No cached checksums found for tileset '{}'.",
    FormatParam{tileset_name, Style::bold}));
err_msg.emplace_back(diag_->formatter().format(
    "Expected to find file '{}'.",
    FormatParam{"porytiles/tilesets/" + tileset_name + "/tileset.cache.json", Style::bold}));
err_msg.emplace_back("Checksum verification requested via configuration.");
std::ranges::copy(
    format_config_note_with_separator(diag_->formatter(), verify_checksums),
    std::back_inserter(err_msg));
return ChainableResult<void>{FormattableError{err_msg}};
```

> _From `compile_primary_tileset.cpp:37-48`._

### Pattern F: Vector of Strings + Vector of Vector of FormatParams (Per-line Params)

Each line has its own `{}` placeholders and its own `std::vector<FormatParam>`. Lines
without params get an empty vector. This is used for complex multi-section errors.

```c++
std::vector<std::string> err_lines;
std::vector<std::vector<FormatParam>> err_params;

// Header line with params
err_lines.emplace_back("Animation '{}' composite subtile '{}': no matching palette found.");
err_params.push_back({FormatParam{anim_name, Style::bold}, FormatParam{tile_idx, Style::bold}});

// Blank separator (no params)
err_lines.emplace_back();
err_params.emplace_back();

// Plain text line (no params)
err_lines.emplace_back("Closest N match(es) with covered colors highlighted:");
err_params.emplace_back();

// Dynamically appended lines
for (const auto &match : matches) {
    err_lines.emplace_back("Palette match candidate: {}");
    err_params.push_back({FormatParam{pal_filename(match.pal_index), Style::bold}});
    for (const auto &line : pal_printer_.print_rgba_palette_covered_missing(/*...*/)) {
        err_lines.push_back(line);
        err_params.emplace_back(); // no params for visualization lines
    }
}

return FormattableError{std::move(err_lines), std::move(err_params)};
```

> _From `primary_tileset_compiler.cpp:874-904`._

### `has_details()` — Checking for Content

```c++
if (formattable_err.has_details()) {
    // Error has at least one non-empty line
}
```

Used internally by `UserDiagnostics::fatal()` to filter out empty passthrough errors.

---

## 4. UserDiagnostics Recipes

All methods require a **tag** string for categorization/filtering.
Each severity has three overloads:
1. `(tag, std::string)` — single-line convenience
2. `(tag, std::vector<std::string>)` — multi-line
3. `(tag, format_str, FormatParam...)` — variadic inline formatting

### 4.1 Remarks

Standalone informational messages about compiler behavior.

```c++
// Single-line (variadic overload)
diag_->remark(
    "base-game-detection",
    "Detected base game '{}'.",
    FormatParam{to_string(detected), Style::bold});

// Multi-line with tile visualization
std::vector<std::string> remark_lines;
remark_lines.push_back(diag_->formatter().format(
    "Mangled tile {} in animation '{}': pixel ({},{}) changed from index {} to {}.",
    FormatParam{i, Style::bold},
    FormatParam{anim_name, Style::bold},
    FormatParam{pixel_row},
    FormatParam{pixel_col},
    FormatParam{mangle_result->second.original_pixel.index()},
    FormatParam{mangle_result->second.mangled_pixel.index()}));
remark_lines.emplace_back("");
remark_lines.emplace_back("Original tile:");
std::ranges::copy(
    tile_printer_->print_tile(original_rgba, extrinsic_transparency),
    std::back_inserter(remark_lines));
remark_lines.emplace_back("Mangled tile:");
std::ranges::copy(
    tile_printer_->print_tile(mangled_rgba, extrinsic_transparency),
    std::back_inserter(remark_lines));
diag_->remark("anim-key-frame-mangle", remark_lines);
```

> _From `base_game_detector.cpp:88` and `anim_key_frame_mangler.cpp:309-333`._

### 4.2 Remark Notes

Always follow a `remark()` call, using the **same tag**.

```c++
// Single-line (variadic overload)
diag_->remark_note(
    "base-game-detection",
    "{} in '{}'.",
    FormatParam{reason},
    FormatParam{global_fieldmap_path.string(), Style::bold});

// Multi-line with config note
diag.remark_note(
    "animation-palette-resolution-strategy",
    format_config_note(diag.formatter(), strategy));
```

> _From `base_game_detector.cpp:90` and `anim_decompiler.cpp:204`._

### 4.3 Warnings

Non-fatal issues the user should know about.

```c++
// Single-line (variadic overload)
diag_->warning(
    "nothing-to-do",
    "Skipping compilation for '{}', no changes found.",
    FormatParam{tileset_name, Style::bold});

// Multi-line
std::vector<std::string> warning_lines;
warning_lines.emplace_back(services.diag.formatter().format(
    "Porymap palette '{}' slot 0 color '{}' does not match extrinsic transparency '{}'.",
    FormatParam{filename, Style::bold},
    FormatParam{slot0_color.to_jasc_str(), Style::bold},
    FormatParam{extrinsic_transparency.value().to_jasc_str(), Style::bold}));
warning_lines.emplace_back("Slot 0 is typically reserved for the transparency color.");
warning_lines.emplace_back("If you are using slot 0 for a .pla blend color, you can ignore this warning.");
services.diag.warning("porymap-palette-slot-0", warning_lines);
```

> _From `primary_tileset_compiler.cpp:96` and `tileset_compile_validators.hpp:287-294`._

### 4.4 Warning Notes

Always follow a `warning()` call, using the **same tag**.

```c++
// Single-line
diag_->warning_note(
    "missing-optional-artifact",
    "All attributes will receive default or inferred values.");

// Multi-line with palette highlight
services.diag.warning_note(
    "porymap-palette-slot-0",
    build_porymap_pal_highlight_lines(
        services.diag.formatter(),
        services.pal_printer,
        "reserved transparency slot",
        pal,
        filename,
        std::vector<std::size_t>{0}));

// Config note as warning_note
services.diag.warning_note(
    "porymap-palette-slot-0",
    format_config_note(services.diag.formatter(), extrinsic_transparency));
```

> _From `tileset_repo.cpp:384`, `tileset_compile_validators.hpp:297-307`._

### 4.5 Errors

Serious issues. The operation will eventually die but may continue to collect more errors.

```c++
// Single-line (variadic overload)
services.diag.error(
    "metatile-limit-exceeded",
    "Too many metatiles ({}) in Porytiles component for tileset '{}'.",
    FormatParam{metatiles.size(), Style::bold},
    FormatParam{tileset_name, Style::bold});

// Multi-line with tile visualization
std::vector<std::string> no_match_err{};
no_match_err.emplace_back(format_.format(
    "{}: no matching tile found.",
    FormatParam{metatile::message_header(format_, metatile_index, layer, subtile), Style::bold}));
std::ranges::copy(
    tile_printer_.print_metatile_tile_highlight(
        porytiles_metatiles_.at(metatile_index), layer, subtile, extrinsic_transparency_),
    std::back_inserter(no_match_err));
diag_.error(tag, no_match_err);
```

> _From `tileset_compile_validators.hpp:227` and `primary_tileset_compiler.cpp:1346`._

### 4.6 Error Notes

Always follow an `error()` call, using the **same tag**.

```c++
// Config note
std::vector<std::string> note_text;
note_text.push_back(
    services.diag.formatter().format("Metatile limit is '{}'.", FormatParam{metatile_limit, Style::bold}));
note_text.emplace_back("");
std::ranges::copy(
    format_config_note(services.diag.formatter(), limit_cfg),
    std::back_inserter(note_text));
services.diag.error_note("metatile-limit-exceeded", note_text);

// Global color limit
services.diag.error_note(
    "global-color-count-violation",
    build_global_color_limit_lines(services.diag.formatter(), count_max, num_pals_cfg));

// Inline string vector literal
services.diag.error_note(
    "transparent-key-frame",
    std::vector<std::string>{
        "Key frame tiles cannot be fully transparent because they would be",
        "indistinguishable from the actual transparent tile at runtime."});

// Palette highlight
services.diag.error_note(
    "porymap-palette-transparency",
    build_porymap_pal_highlight_lines(
        services.diag.formatter(),
        services.pal_printer,
        "slots with invalid extrinsic transparency",
        pal,
        filename,
        violating_slots));
```

> _From `tileset_compile_validators.hpp:233-238, 1245, 1352, 331`._

### 4.7 Fatal Errors (Error Chain Visualization)

`fatal()` takes a failed `ChainableResult` and visualizes the full error chain.

```c++
if (!result.has_value()) {
    diag_->fatal(result);
    // program typically exits after this
}
```

The chain is rendered as:
- **Proximate**: the immediate error (always present)
- **Steps**: intermediate causes (if chain > 2 errors)
- **Root**: the original cause (if chain > 1 error)

Empty `FormattableError` entries (from passthrough macros) are automatically filtered out.

---

## 5. Diagnostic Stencils Recipes

Reusable helpers in `diagnostic_stencils.hpp` that return `std::vector<std::string>` ready
to pass to any diagnostic method.

### `format_config_note`

Shows how a config value got its current setting.

```c++
// Returns:
//   "'num_pals_in_primary' is '12' due to configuration:"
//   ""
//   <prettified config source lines>

std::vector<std::string> lines = format_config_note(formatter, config_value);

// Typical usage: as a note following a warning or error
services.diag.error_note("some-tag", format_config_note(services.diag.formatter(), limit_cfg));

// Or appended into an existing vector
std::ranges::copy(
    format_config_note(services.diag.formatter(), extrinsic_transparency),
    std::back_inserter(note_text));
```

### `format_config_note_with_separator`

Same as above but with a visual separator above. Used when the config note follows other
information inside the same message.

```c++
// Returns:
//   ""
//   "--------"
//   ""
//   "'verify-checksums' is 'true' due to configuration:"
//   ""
//   <prettified config source lines>

std::ranges::copy(
    format_config_note_with_separator(diag_->formatter(), verify_checksums),
    std::back_inserter(err_msg));
```

> _From `compile_primary_tileset.cpp:44` and `anim_decompiler.cpp:214`._

### `build_global_color_limit_lines`

Explains how the global color count limit is calculated.

```c++
// Returns:
//   "Global unique color limit is '180'."
//   ""
//   "Color limit definition:"
//   "num_pals_in_primary * nontransparent_colors_per_pal:"
//   "12 * 15 = 180"
//   ""
//   <format_config_note for num_pals>

services.diag.error_note(
    "global-color-count-violation",
    build_global_color_limit_lines(services.diag.formatter(), count_max, num_pals_cfg));
```

> _From `tileset_compile_validators.hpp:1245`._

### `build_porymap_pal_highlight_lines` / `build_porytiles_pal_highlight_lines`

Render a palette with specific slots visually highlighted.

```c++
// Porymap palette variant
services.diag.error_note(
    "porymap-palette-transparency",
    build_porymap_pal_highlight_lines(
        services.diag.formatter(),
        services.pal_printer,
        "slots with invalid extrinsic transparency",  // message
        pal,                                           // Palette<Rgba32, N>
        filename,                                      // label e.g. "0.pal"
        violating_slots));                             // std::vector<std::size_t>

// Porytiles palette variant
services.diag.warning_note(
    "unused-manual-color",
    build_porytiles_pal_highlight_lines(
        services.diag.formatter(),
        services.pal_printer,
        "slots with unused colors",
        pal,
        filename,
        unused_slots));
```

> _From `tileset_compile_validators.hpp:331` and `tileset_compile_validators.hpp:1149`._

### `build_pal_hint_highlight_lines`

Same idea but for palette hints (user-provided color specifications).

```c++
services.diag.error_note(
    "pal-hint-violation",
    build_pal_hint_highlight_lines(
        services.diag.formatter(),
        services.pal_printer,
        "conflicting colors",
        hint,           // PaletteHint
        pal_label,      // std::string
        violating_slots));
```

---

## 6. Composite Patterns

### Error + Note + Config Note (Most Common Pattern)

```c++
constexpr auto tag = "metatile-limit-exceeded";

// 1. Emit the error
services.diag.error(
    tag,
    "Too many metatiles ({}) in Porytiles component for tileset '{}'.",
    FormatParam{metatiles.size(), Style::bold},
    FormatParam{tileset_name, Style::bold});

// 2. Emit a note with the limit + config context
std::vector<std::string> note_text;
note_text.push_back(
    services.diag.formatter().format("Metatile limit is '{}'.", FormatParam{metatile_limit, Style::bold}));
note_text.emplace_back("");
note_text.append_range(format_config_note(services.diag.formatter(), limit_cfg));
services.diag.error_note(tag, note_text);
```

> _From `tileset_compile_validators.hpp:227-238`._

### Warning + Palette Highlight Note + Config Note (Stacking Multiple Notes)

```c++
constexpr auto tag = "porymap-palette-slot-0";

// 1. Warning
services.diag.warning(tag, warning_lines);

// 2. First note: palette visualization
services.diag.warning_note(
    tag,
    build_porymap_pal_highlight_lines(
        services.diag.formatter(), services.pal_printer,
        "reserved transparency slot", pal, filename,
        std::vector<std::size_t>{0}));

// 3. Second note: config context
services.diag.warning_note(
    tag,
    format_config_note(services.diag.formatter(), extrinsic_transparency));
```

> _From `tileset_compile_validators.hpp:287-307`._

### Multi-line FormattableError with Bullet Points + Config Separator

```c++
std::vector<std::string> err_msg{};
err_msg.emplace_back("Changes present in Porymap assets:");
for (const auto &key : mismatched_keys) {
    err_msg.emplace_back(diag_->formatter().format(
        "  {}",
        FormatParam{key.key(), Style::bold}));
}
err_msg.emplace_back("");
err_msg.emplace_back("Compiling now would clobber your Porymap asset changes.");
err_msg.emplace_back("To resolve:");
err_msg.emplace_back(diag_->formatter().format(
    "  - Run '{} {}' to synchronize assets.",
    FormatParam{"decompile-tileset", Style::bold},
    FormatParam{tileset_name, Style::bold}));
err_msg.emplace_back(diag_->formatter().format(
    "  - {} disable checksum verification to allow the clobber.",
    FormatParam{"OR", Style::bold}));
err_msg.emplace_back(diag_->formatter().format(
    "  - {} delete '{}' cache file.",
    FormatParam{"OR", Style::bold},
    FormatParam{"porytiles/tilesets/" + tileset_name + "/tileset.cache.json", Style::bold}));
std::ranges::copy(
    format_config_note_with_separator(diag_->formatter(), verify_checksums),
    std::back_inserter(err_msg));
return ChainableResult<void>{FormattableError{err_msg}};
```

> _From `compile_primary_tileset.cpp:63-84`._

### Remark + Remark Note (Simple Pair)

```c++
constexpr auto tag = "base-game-detection";
diag_->remark(tag, format_->format(
    "Detected base game '{}'.", FormatParam{to_string(detected), Style::bold}));
diag_->remark_note(tag, format_->format(
    "{} in '{}'.", FormatParam{reason}, FormatParam{global_fieldmap_path.string(), Style::bold}));
```

> _From `base_game_detector.cpp:88-93`._

---

## 7. Error Chain Macros

All macros live in `chainable_result.hpp`. They reduce the common check-and-return pattern
from 6 lines to 1.

### `PT_TRY_ASSIGN_CHAIN_ERR` — Unwrap with New Error Context

Evaluates an expression returning `ChainableResult<T>`. On error, returns early with a new
`FormattableError` chained onto the existing error chain.

```c++
// Signature: PT_TRY_ASSIGN_CHAIN_ERR(var, expr, return_type, ...)
// The variadic args are forwarded to FormattableError(...).

// Simple string message
PT_TRY_ASSIGN_CHAIN_ERR(
    metatiles_key,
    key_provider_->key_for_metatiles_bin(tileset.name()),
    void,
    "Tileset save failed.");
// `metatiles_key` now holds the unwrapped value

// FormatParam message (use parens, not braces, inside macro calls)
PT_TRY_ASSIGN_CHAIN_ERR(
    metatiles_key,
    key_provider_->key_for_metatiles_bin(tileset->name()),
    std::unique_ptr<Tileset>,
    "Failed to load tileset '{}'.", FormatParam(tileset->name(), Style::bold));

// Vector of strings message (built inline)
PT_TRY_ASSIGN_CHAIN_ERR(
    match,
    internal_png_pal_strategy(anim, pals, extrinsic_transparency, diag, pal_printer),
    std::size_t,
    err_msg);     // a pre-built std::vector<std::string>
```

> _From `tileset_repo.cpp:28`, `tileset_repo.cpp:219`, `anim_decompiler.cpp:236`._

### `PT_TRY_ASSIGN_PASS_ERR` — Unwrap, Passthrough (Different Types)

When the current layer adds no context and the inner/outer success types differ.
Chains an empty `FormattableError` (filtered out by `fatal()`).

```c++
// Signature: PT_TRY_ASSIGN_PASS_ERR(var, expr, return_type)
PT_TRY_ASSIGN_PASS_ERR(tileset, load_tileset(name), std::unique_ptr<CompiledTileset>);
```

### `PT_TRY_ASSIGN_PASS_SAME_ERR` — Unwrap, Passthrough (Same Types)

When the inner and outer `ChainableResult` types match exactly. Returns the error unchanged.

```c++
// Signature: PT_TRY_ASSIGN_PASS_SAME_ERR(var, expr)
PT_TRY_ASSIGN_PASS_SAME_ERR(result, some_function_returning_same_type());
```

### `PT_TRY_CALL_CHAIN_ERR` — Void Call with New Error Context

For `ChainableResult<void>` expressions. Same as `PT_TRY_ASSIGN_CHAIN_ERR` but no variable
assignment.

```c++
// Signature: PT_TRY_CALL_CHAIN_ERR(expr, return_type, ...)
// The variadic args are forwarded to FormattableError(...).

// Simple string message
PT_TRY_CALL_CHAIN_ERR(
    pipeline_helper_register_animations(),
    void,
    "Failed to register animations.");

// FormatParam message (use parens, not braces, inside macro calls)
PT_TRY_CALL_CHAIN_ERR(
    tileset_repo_->save(*compiled_tileset),
    void,
    "Failed to save tileset '{}'.", FormatParam(tileset_name, Style::bold));
```

> _From `primary_tileset_compiler.cpp:419`._

### `PT_TRY_CALL_PASS_ERR` — Void Call, Passthrough (Different Types)

```c++
// Signature: PT_TRY_CALL_PASS_ERR(expr, return_type)
PT_TRY_CALL_PASS_ERR(
    validate_metatile_count(services, tileset_.name(), false, porytiles_metatiles_),
    void);
```

> _From `primary_tileset_compiler.cpp:305`._

### `PT_TRY_CALL_PASS_SAME_ERR` — Void Call, Passthrough (Same Types)

```c++
// Signature: PT_TRY_CALL_PASS_SAME_ERR(expr)
PT_TRY_CALL_PASS_SAME_ERR(
    pipeline_step_process_porytiles_input());
```

### Decision Guide: Which Macro?

| Scenario | Has Value? | Types Match? | Macro |
|----------|------------|-------------|-------|
| Unwrap value, add context | Yes | N/A | `PT_TRY_ASSIGN_CHAIN_ERR` |
| Unwrap value, no context, types differ | Yes | No | `PT_TRY_ASSIGN_PASS_ERR` |
| Unwrap value, no context, types match | Yes | Yes | `PT_TRY_ASSIGN_PASS_SAME_ERR` |
| Void call, add context | No | N/A | `PT_TRY_CALL_CHAIN_ERR` |
| Void call, no context, types differ | No | No | `PT_TRY_CALL_PASS_ERR` |
| Void call, no context, types match | No | Yes | `PT_TRY_CALL_PASS_SAME_ERR` |

---

## Message Style Rules Reminder

All user-facing error and diagnostic messages **must** follow these rules:

1. **Capital first letter** — `"Failed to read..."` not `"failed to read..."`
2. **End with a period** — `"Tileset does not exist."` not `"Tileset does not exist"`
3. **Single quotes + `Style::bold`** around highlightable items — `FormatParam{name, Style::bold}` with `'{}'`
4. **List headers ending with `:`** are fine — `"To resolve:"`
5. **Bullet sub-items** follow their own sub-style — `"  - Run '{}' to synchronize."`
