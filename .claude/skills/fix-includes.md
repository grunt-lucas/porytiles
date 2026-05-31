---
name: fix-includes
description: Fix relative include paths in Porytiles source files back to absolute-style includes.
user_invocable: true
---

Fix all relative include paths in Porytiles source files (.hpp, .cpp, and .ipp).

## Problem

IDE refactoring tools sometimes convert absolute-style includes into relative paths. For example:
- `#include "../../infra/models/foo.hpp"` should be `#include "porytiles/infra/models/foo.hpp"`
- `#include "../../../include/porytiles/domain/bar.hpp"` should be `#include "porytiles/domain/bar.hpp"`

## Task

1. Search for all `#include` directives in `Porytiles/**/*.hpp`, `Porytiles/**/*.cpp`, and `Porytiles/**/*.ipp` that use relative paths (containing `../`)
2. For each file with relative includes, convert them to the proper absolute-style format:
   - All includes should start with `porytiles/` followed by the appropriate path
   - Remove any `../` or `../../` prefixes
   - Remove any `include/porytiles/` patterns and replace with just `porytiles/`
3. Do NOT modify includes for external libraries (like fmt, gsl, etc.) or standard library headers

## Rules

- Only fix includes that match the relative path pattern (containing `../`)
- Preserve the order of includes within each file
- Do not add or remove any includes, only fix the paths
- After fixing, run the format script: `uv run Scripts/format.py`

Report which files were modified when done.
