# C Parser Architecture Analysis: AST vs Token Pattern Matching

**Document Version:** 1.0
**Created:** 2025-12-27
**Target Component:** `Porytiles2/utilities/c_parser/`

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Current Architecture Assessment](#current-architecture-assessment)
3. [Pain Points Analysis](#pain-points-analysis)
4. [Decision Framework](#decision-framework)
5. [Library Options](#library-options)
6. [Recommended Approach](#recommended-approach)
7. [Implementation Details](#implementation-details)

---

## Executive Summary

The Porytiles C parser is a **targeted pattern-matching system** that works well for declaration-level parsing (defines, enums, structs, arrays) but struggles with statement/expression-level parsing. The pain is currently **localized to AnimCodeParser**, which performs 8+ helper functions of manual token pattern matching.

### Key Finding

Given that **users customize their animation code** (not just standard pokeemerald patterns), the token pattern-matching approach is fundamentally limited. You cannot enumerate all possible patterns users might write.

### Recommendation

Adopt **tree-sitter-c** for statement/expression parsing while keeping the current parser for simple declaration extraction.

---

## Current Architecture Assessment

### Three-Stage Compiler Architecture

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   Lexer     │───▶│   Parser    │───▶│   Facade    │
│ (tokenize)  │    │ (extract)   │    │ (orchestrate)│
└─────────────┘    └─────────────┘    └─────────────┘
```

### What the Parser Handles

| Construct | Method | Output |
|-----------|--------|--------|
| `#define` statements | `parse_defines()` | `DefineStatement` with evaluated value |
| Enum declarations | `parse_enums()` | `EnumDeclaration` with members |
| Pointer arrays | `parse_pointer_arrays()` | `ArrayDeclaration` with elements |
| Function definitions | `parse_functions()` | `FunctionDefinition` with **raw body tokens** |
| Struct variables | `parse_struct_variables()` | `StructVariableDeclaration` |
| Struct initializers | `parse_struct_initializers()` | `StructInitializerDeclaration` with fields |
| INCBIN arrays | `parse_incbin_arrays()` | `IncbinDeclaration` with paths |

### Consumer Analysis

| Consumer | Parsing Method | Works Well? |
|----------|---------------|-------------|
| HeaderBehaviorMapProvider | `parse_defines()`, `parse_enums()` | ✅ Simple |
| ProjectTilesetMetadataProvider | `parse_struct_initializers()` | ✅ Simple |
| ProjectTilesetMetadataProvider | `parse_struct_variables()` | ✅ Simple |
| **AnimCodeParser** | Manual token pattern matching | ❌ 8+ helper functions |

**Key Insight:** Only AnimCodeParser suffers because it needs to understand function bodies (statements and expressions), not just declarations.

---

## Pain Points Analysis

### The AnimCodeParser Problem

AnimCodeParser receives `FunctionDefinition::body_tokens()` and must implement manual token pattern matching to extract:

1. Driver function from callback: `sPrimaryTilesetAnimCallback = <func>`
2. Timer conditions: `if (timer % X == Y)`
3. Function calls: `AppendTilesetAnimToBuffer(...)`
4. Expression arguments: `TILE_OFFSET_4BPP(X)`, `4 * TILE_SIZE_4BPP`

### Current Pattern Matching (Brittle)

```cpp
// Example from anim_code_parser.cpp:105-115
[[nodiscard]] std::optional<std::size_t> extract_tile_offset(const std::vector<Token> &tokens)
{
    for (std::size_t i = 0; i + 3 < tokens.size(); ++i) {
        if (tokens[i].is(TokenType::identifier) && tokens[i].text() == "TILE_OFFSET_4BPP" &&
            tokens[i + 1].is(TokenType::left_paren) && tokens[i + 2].is(TokenType::integer_literal) &&
            tokens[i + 3].is(TokenType::right_paren)) {
            return tokens[i + 2].int_value();
        }
    }
    return std::nullopt;
}
```

### Problems with This Approach

| Issue | Example | Consequence |
|-------|---------|-------------|
| **Brittle patterns** | `timer % 8 == 0` works, but `(timer % 8) == 0` may not | Silent failures |
| **Manual index management** | `i + 3 < tokens.size()` | Off-by-one errors |
| **No expression evaluation** | `4 * TILE_SIZE_4BPP` requires token-by-token matching | Can't handle variations |
| **Linear search** | Scanning entire function body | Performance, complexity |
| **No semantic understanding** | Can't distinguish scopes | May match wrong patterns |

### Helper Functions in AnimCodeParser

1. `extract_anim_name_from_array_ref()` - string parsing on identifiers
2. `extract_frame_indices()` - string parsing on element names
3. `extract_tile_offset()` - searches for `TILE_OFFSET_4BPP(X)`
4. `extract_tile_count()` - searches for `X * TILE_SIZE_4BPP`
5. `find_driver_function_from_callback()` - searches for callback assignment
6. `extract_timer_conditions()` - searches for `timer % X == Y`
7. `extract_array_name_from_first_arg()` - extracts identifier from subscript
8. `find_function_calls()` - pattern matches function calls in tokens

---

## Decision Framework

### Stay with Current Approach If:

- AnimCodeParser remains the **only** consumer needing statement/expression parsing
- Patterns are **finite and well-defined** (pokeemerald-specific only)
- You can **enumerate all patterns** you'll ever need
- You're not parsing **user-written** C code

### Switch to AST If Any Become True:

1. ✅ Need to parse **arbitrary expressions** (not just known patterns)
2. ⬜ Need to understand **control flow** (nested if/else, loops, switch)
3. ⬜ Need **type resolution** (pointer vs array, struct field types)
4. ⬜ **Multiple new consumers** need statement-level parsing
5. ⬜ Token pattern-matching code exceeds **~15-20 helper functions**
6. ✅ Need to handle **user-customized animation code**

**Current Status:** Items 1 and 6 are already true based on discussion. This crosses the threshold.

---

## Library Options

### Tier 1: Minimal Expression AST on Current Lexer

**Approach:** Build small AST classes on top of existing lexer

```cpp
struct BinaryExpr { Token left; Token op; Token right; };
struct MacroCall { std::string name; std::vector<Token> args; };
struct AssignmentStmt { Token lhs; Token rhs; };
```

| Pros | Cons |
|------|------|
| Incremental improvement | Development effort |
| Reuse existing lexer | Still limited to known patterns |
| No new dependencies | Need to handle C grammar edge cases |
| Full control | Won't handle arbitrary user code |

**Verdict:** Viable for pokeemerald-only patterns, insufficient for user customization.

---

### Tier 2: tree-sitter-c (Recommended)

**Best balance of capability and simplicity**

```cmake
FetchContent_Declare(
  tree_sitter
  GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
  GIT_TAG v0.22.6
)
FetchContent_Declare(
  tree_sitter_c
  GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-c.git
  GIT_TAG v0.21.4
)
FetchContent_MakeAvailable(tree_sitter tree_sitter_c)
```

| Pros | Cons |
|------|------|
| Full C grammar, handles any valid code | CST not AST (preserves all tokens) |
| Incremental parsing (fast re-parse) | Runtime library ~500KB |
| Well-maintained, battle-tested | Learning curve for tree-sitter API |
| Visitor-style traversal | C API (need C++ wrapper) |
| FetchContent compatible | |

**Verdict:** Best choice for Porytiles' needs.

---

### Tier 3: PEGTL (Header-Only PEG)

**Define grammar in C++ templates**

```cmake
FetchContent_Declare(
  pegtl
  GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
  GIT_TAG 3.2.7
)
```

| Pros | Cons |
|------|------|
| Header-only, zero runtime | Must write C grammar yourself |
| C++17, integrates naturally | PEG learning curve |
| Generates efficient parsers | Complex template error messages |

**Verdict:** Good for custom grammars, overkill for standard C parsing.

---

### Tier 4: libclang (Full Semantic Analysis)

| Pros | Cons |
|------|------|
| Full C semantic analysis | MASSIVE dependency (LLVM) |
| Handles all edge cases | Complex API |
| Type resolution, macro expansion | Impractical for FetchContent |

**Verdict:** Only if you need type information or macro expansion. Overkill.

---

## Recommended Approach

### Migration Strategy

**Phase 1: Add tree-sitter alongside existing parser**
- Keep current parser for simple declaration extraction
- Use tree-sitter for AnimCodeParser's statement/expression needs
- Both parsers coexist

**Phase 2: Gradually migrate**
- Move more parsing to tree-sitter as needed
- Evaluate whether to keep or deprecate current lexer/parser

**Phase 3: Full tree-sitter (optional)**
- If tree-sitter proves valuable, migrate remaining parsing
- Keep current parser only if there's specific value

### What Changes

| Component | Action |
|-----------|--------|
| `c_parser/lexer.hpp` | Keep for now |
| `c_parser/parser.hpp` | Keep for declaration parsing |
| `c_parser/c_parser_facade.hpp` | Keep, possibly add tree-sitter methods |
| AnimCodeParser | Refactor to use tree-sitter for function body analysis |

---

## Implementation Details

### New Files

```
Porytiles2/
├── include/porytiles2/utilities/tree_sitter/
│   ├── tree_sitter_parser.hpp      # Main parser interface
│   └── ast_nodes.hpp               # C++ wrappers for tree-sitter nodes
└── lib/utilities/tree_sitter/
    └── tree_sitter_parser.cpp      # Implementation
```

### Interface Sketch

```cpp
namespace porytiles2 {

/**
 * @brief C source parser using tree-sitter for full syntax tree support.
 *
 * @details
 * Provides AST-level access to C source code, enabling pattern matching
 * on function bodies, expressions, and statements that the simpler
 * token-based parser cannot handle.
 */
class TreeSitterParser {
  public:
    explicit TreeSitterParser(const std::filesystem::path &file);

    /**
     * @brief Find all function calls matching a specific function name.
     *
     * @param name The function name to search for
     * @return Vector of function call nodes with their arguments
     */
    [[nodiscard]] std::vector<FunctionCallNode> find_function_calls(std::string_view name) const;

    /**
     * @brief Find all assignments to a specific variable.
     *
     * @param lhs The left-hand side variable name
     * @return Vector of assignment nodes
     */
    [[nodiscard]] std::vector<AssignmentNode> find_assignments(std::string_view lhs) const;

    /**
     * @brief Find all if statements in the source.
     *
     * @return Vector of if statement nodes with conditions and bodies
     */
    [[nodiscard]] std::vector<IfStatementNode> find_if_statements() const;

    /**
     * @brief Evaluate a simple arithmetic expression.
     *
     * @param expr The expression node to evaluate
     * @return The evaluated value, or nullopt if not evaluable
     */
    [[nodiscard]] std::optional<std::int64_t> evaluate_expression(const ExpressionNode &expr) const;

  private:
    // tree-sitter state
    TSParser *parser_;
    TSTree *tree_;
    std::string source_;
};

} // namespace porytiles2
```

### CMake Integration

```cmake
# In Porytiles2/CMakeLists.txt

include(FetchContent)

# tree-sitter core runtime
FetchContent_Declare(
  tree_sitter
  GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
  GIT_TAG v0.22.6
)

# C grammar for tree-sitter
FetchContent_Declare(
  tree_sitter_c
  GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-c.git
  GIT_TAG v0.21.4
)

FetchContent_MakeAvailable(tree_sitter tree_sitter_c)

# Link to your target
target_link_libraries(Porytiles2Lib
  PRIVATE
    tree-sitter
    tree-sitter-c
)
```

### Refactored AnimCodeParser (Conceptual)

```cpp
// Before: Manual token pattern matching
std::string driver_func_name = find_driver_function_from_callback(callback_func.body_tokens());

// After: Tree-sitter query
TreeSitterParser ts_parser{c_file_path};
auto assignments = ts_parser.find_assignments("sPrimaryTilesetAnimCallback");
if (!assignments.empty()) {
    driver_func_name = assignments.front().rhs().as_identifier();
}
```

---

## Summary

| Aspect | Current State | Recommended State |
|--------|---------------|-------------------|
| Declaration parsing | ✅ Works well | Keep as-is |
| Function body parsing | ❌ Manual token matching | tree-sitter |
| User code support | ❌ Pattern enumeration | ✅ Full C grammar |
| Maintenance burden | Growing | Reduced |
| New dependency | None | tree-sitter (~500KB) |

The investment in tree-sitter pays off when:
1. Users customize animation code
2. More consumers need statement/expression parsing
3. The token pattern-matching helpers exceed maintainability

Given that condition #1 is already true, adopting tree-sitter is justified.
