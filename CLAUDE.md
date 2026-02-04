# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

Porytiles is a C++ overworld tileset compiler for Pokémon Generation III decompilation projects. It takes RGBA input assets and generates Porymap-ready binary assets (metatiles.bin, metatile_attributes.bin, tiles.png, palettes).

## Architecture

The project is organized into two main versions:
- **Porytiles1**: Legacy version (ignore unless explicitly instructed)
- **Porytiles2**: Active development with domain-driven design architecture inspired by clang

Key directories in Porytiles2:
- `Documentation/` - Documentation source folder
- `Porytiles2/config_templates/` - Code generation for config system
- `Porytiles2/include/porytiles2/domain/` - Pure business logic, no I/O dependencies
- `Porytiles2/include/porytiles2/app/` - User-facing use cases and workflows
- `Porytiles2/include/porytiles2/infra/` - I/O and external system/library integration
- `Porytiles2/include/porytiles2/xcut/` - Cross-cutting concerns (errors, diagnostics, config, di, etc)
- `Porytiles2/include/porytiles2/utilities/` - Generic helpers, zero dependencies
- `Porytiles2/lib/` - .cpp implementation files
- `Porytiles2/Notes/` - WIP documentation, feature outlines, refactoring plans, etc
- `Porytiles2/tests/` - GoogleTest test suites
- `Porytiles2/scratch/` - My scratch directory, you can ignore this
- `Porytiles2/tools/` - Tools that use the Porytiles library, currently just the main CLI tool
- `Resources/` - Test assets and example files
- `Scripts/` - Utility scripts for the repository (including config system generation)

## Specialized Agents

This project has custom Claude Code agents in `.claude/agents/` for specialized tasks:

**Development:**
- **build-expert**: CMake builds, compilation errors, linker issues
- **test-runner**: GoogleTest execution, test failures, adding tests
- **debugger**: Runtime errors, crashes, logic bugs

**Code Quality:**
- **code-reviewer**: Code quality, style compliance, security review
- **documenter**: Doxygen documentation, proper tag ordering
- **refactorer**: Safe code refactoring, symbol renaming, code moves

**Architecture:**
- **architect**: DDD layer decisions, component placement, dependency rules
- **di-expert**: Fruit dependency injection, wiring, DI debugging
- **config-generator**: Config schema, Jinja2 templates, code generation
- **migrator**: Porting code from Porytiles1 to Porytiles2

Use these agents for complex tasks in their domains.

## Build System

Uses CMake 3.20+ with C++23. Requires `zlib`, `libpng`, and GoogleTest.

**CRITICAL**: Build directories are `clion-build-debug` or `vscode-build` (NEVER `build`).

Quick reference:
```bash
cmake --build clion-build-debug -j7 > /tmp/build.log 2>&1  # Build (check exit code)
cmake --install clion-build-debug --prefix ~/.local        # Install to ~/.local/bin
./clion-build-debug/Porytiles2/tests/Porytiles2AllTests > /tmp/test.log 2>&1  # Test
./Scripts/format.sh 2> /dev/null  # Format code
```

**After building, always install the executable to make it available for testing.**

## Testing with pokeemerald-expansion

A pokeemerald-expansion testbed project is available at `../pokeemerald-expansion` for testing the installed `porytiles2` executable against a real decomp project.
You can either `cd` into the testbed project or run `porytiles2` with the `--project-root` set to `../pokeemerald-expansion`.

## Documentation Repositories

Porytiles has two separate documentation repositories (gitignored in the main repo):

- **porytiles-user-docs/**: User-facing documentation (tutorials, CLI reference, usage guides)
  - URL: https://grunt-lucas.github.io/porytiles-user-docs/
  - GitHub: https://github.com/grunt-lucas/porytiles-user-docs

- **porytiles-dev-docs/**: Developer documentation (architecture, contributing, API)
  - URL: https://grunt-lucas.github.io/porytiles-dev-docs/
  - GitHub: https://github.com/grunt-lucas/porytiles-dev-docs

These are **separate git repositories** cloned into the main porytiles directory. They follow [porymap's documentation pattern](https://github.com/huderlem/porymap) with Sphinx and the Read the Docs theme.

### Building Documentation

```bash
# Navigate to the docs repo
cd porytiles-user-docs  # or porytiles-dev-docs

# Build HTML locally
cd docsrc && uv run make html

# Build and deploy to docs/ for GitHub Pages
cd docsrc && uv run make github
```

After running `make github`, commit and push the changes in the docs repo to deploy to GitHub Pages.

## Python Environment For Config System Code Generation

**CRITICAL: Use `uv` for Python script execution!**

Porytiles2 uses [uv](https://docs.astral.sh/uv/) for Python dependency management.
Install uv if you haven't: https://docs.astral.sh/uv/getting-started/installation/

```bash
# Regenerate config files (after modifying config_schema.yaml or .jinja2 templates)
uv run Scripts/generate_config.py
```

That's it - `uv run` automatically handles dependencies from `pyproject.toml`.

## C++ Code Style

Follow the style guide in @./STYLE.md

## **CRITICAL RULES - DO NOT VIOLATE**

### Behavioral Rules
- **Ignore `Porytiles1/`** unless explicitly told to work with those files
- **NEVER create mock data or simplified components** unless explicitly told to
- **NEVER replace existing complex components with simplified versions** - fix the actual problem
- **ALWAYS find and fix the root cause** of issues instead of creating workarounds
- When something doesn't work, debug and fix it - **don't start over with a simple version**

### Code Style Rules
- **ALWAYS use `uv run`** when running Python scripts
- **ALWAYS follow the code style** in STYLE.md
- Use braced initialization where possible (but avoid when ambiguous constructors exist)
- **Never** include headers using relative paths
- Follow const correctness principles
- Always use namespace `porytiles2`, no child namespaces (unless explicitly instructed)
- Place private helper functions in **anonymous namespaces in .cpp files**, not in class `private:` sections
- Code must work on **both GCC and Clang** - no compiler-specific code

### Context Management
- Send build/test output to `/tmp` files to preserve context
- Check exit codes to validate success before inspecting output
