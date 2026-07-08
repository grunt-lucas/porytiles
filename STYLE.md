# Porytiles C++ Style Guide

- [Porytiles C++ Style Guide](#porytiles-c-style-guide)
  - [Naming, Includes, and Layout](#naming-includes-and-layout)
    - [Trailing Underscores: Class-Private Members Only](#trailing-underscores-class-private-members-only)
  - [Documentation Comments](#documentation-comments)
    - [How Much to Document](#how-much-to-document)
    - [Doxygen Comment Style](#doxygen-comment-style)
    - [Internal Helpers and Skipping](#internal-helpers-and-skipping)
  - [Test Style](#test-style)
  - [Idioms and Patterns](#idioms-and-patterns)
    - [Container Membership Checks](#container-membership-checks)
    - [Bounds-Checked Element Access](#bounds-checked-element-access)
    - [std::formatter Specializations](#stdformatter-specializations)
  - [Error and Diagnostic Message Style](#error-and-diagnostic-message-style)
  - [Prose and Markdown](#prose-and-markdown)
    - [Prose Style](#prose-style)
    - [Markdown Code Blocks](#markdown-code-blocks)

This is the C++ style guide for Porytiles.
It covers naming and layout, documentation comments, tests, idioms,
error messages, and prose conventions.
Follow it for all code in the active `porytiles/` tree;
the `legacy/` tree predates these rules and is exempt.

---

## Naming, Includes, and Layout

The annotated example below shows the core conventions:
include ordering, the single `porytiles` namespace, naming for each kind of
identifier, and class layout.
The later sections expand on specific topics.

```c++
// First include should always be declaration header, if relevant
#include "porytiles/domain/MyClass.hpp"

// Next, include C++ stdlib headers with angle brackets
#include <string>
#include <vector>

// Next, include project libraries with quotes
#include "gsl/pointers"
#include "yaml-cpp/yaml.h"

// Finally, include other Porytiles headers with quotes
#include "porytiles/domain/foo.hpp"

// Notice that between each include group, we place an extra newline

// Namespace for all Porytiles code should be "porytiles", never use a child namespace
namespace porytiles {

// PascalCase for enum class names
enum class FooBar {
    // snake_case for the actual constants
    foo_value_1,
    foo_value_2
};

// snake_case for global constants
const std::string foo_bar_value_1 = "foo_value_1";

// PascalCase for class names
class MyClass {
  public:
    MyClass() = default;

    // ctor initializer lists always use braced initialization where possible
    // simple ctors can be implemented in the header file
    MyClass(int my_val) : my_val_{my_val} {}

    // class constants use snake_case
    const std::string my_class_constant = "my_class_constant";

    // Method names are snake_case, parameter names are snake_case
    // Use [[nodiscard]] for methods/functions that return a value
    [[nodiscard]] int compute_something(int accum_value) const;

    // Do something complicated to update my_val_
    // This should be implemented in the cpp file
    void update_my_val_with_complex_process(int some_param);

    // Simple accessors/mutators also use snake_case, but omit the trailing underscore
    // Simple accessors/mutators can be implemented in the header file
    [[nodiscard]] const std::string &cool_value() const {
        return cool_value_;
    }

    [[nodiscard]] int my_val() const {
        return my_val_;
    }

    void my_val(int new_val) {
        my_val_ = new_val;
    }

  private:
    // Member variables use snake_case_ with trailing underscore
    std::string cool_value_;
    int my_val_;
};

// cpp file implementations
int MyClass::compute_something(int accum_value) const {
    // All comments *must* use the two-slash style.

    // Multi-line comments lik this one should also
    // use the two-slash style.

    // local variable names are snake_case
    int my_local = 1;
    return my_local + my_val_ + accum_value;
}

} // namespace porytiles

// Close a namespace with a closing comment like above
```

### Trailing Underscores: Class-Private Members Only

The trailing `_` convention applies **only to class-private member variables**.
Public struct fields (record-style data carriers like `LayerValue`, `FrameLoadResult`, `CliOptionStorage`, `PerAnimOverride`, etc.) use plain snake_case with **no trailing underscore**:

```c++
// CORRECT — public struct fields, no trailing underscore:
struct LayerValue {
    std::optional<T> value;
    std::string source_key;
    ValidationState state;
};

// CORRECT — class with private members, trailing underscore:
class MyClass {
  private:
    int my_val_;
    std::string cool_value_;
};

// WRONG — public struct field with trailing underscore:
struct PackingParams {
    std::vector<PixelTile<Rgba32>> tiles_; // should be `tiles`
};
```

If a struct has a constructor and is used in a class-like way (treating its members as private state),
prefer making it a `class` with private members rather than a public struct that imitates the private-member convention.

---

## Documentation Comments

### How Much to Document

How *much* Doxygen a declaration gets depends on its visibility and complexity.
Apply these three tiers:

1. **Public API.**
   Methods, classes, and free functions visible to developers as part of the Porytiles API
   always get a full Doxygen block: the complete tag set in the idiomatic order shown under
   [Doxygen Comment Style](#doxygen-comment-style).
   The one exception is trivially simple members (plain accessors, mutators, and thin wrappers
   whose signature says everything), which follow tier 3 and stay undocumented.
   A tautological block is **worse** than none.
2. **Internal helpers.**
   Anonymous-namespace helpers and private class methods carry only `@brief` by default,
   plus `@details` when the helper is complex enough to need it.
   Omit the procedural tags (`@param`, `@return`, `@pre`, `@post`, `@tparam`, etc.) that tier 1 requires.
   No external caller reads these, so the extra tags are visual noise without payoff.
3. **Too simple to document.**
   When a helper is so simple that a doc comment would just restate the code, write no Doxygen at all.
   Add a comment only when a subcall is genuinely unintuitive or a key invariant needs explicit callout.

The two sections below show the full block (tier 1) and the trimmed or absent forms (tiers 2 and 3).

### Doxygen Comment Style

```c++
// Always use @brief and @details
/**
 * @brief A basic class for for modeling foos.
 *
 * @details
 * The Foo class assumes that your foos are all like bars, but different. Notice that the comment goes all the way to
 * the column limit of 120 before wrapping.
 *
 * @tparam T The type parameter for the foo
 * @invariant Some note would go here
 */
template <typename T>
class Foo {
  public:
   // NOTICE:
   // a blank line between @brief and @details
   // a blank line between @details and the other doc tags
   //
   // IDIOMATIC TAG ORDER:
   // 1. @brief
   // 2. @details
   // 3. @tparam (for templates)
   // 4. @invariant (only relevant for structs/classes, condition that is true at all times in object lifecycle)
   // 5. @param (for parameters)
   // 6. @pre (preconditions - what must be true BEFORE calling)
   // 7. @return (what the function returns)
   // 8. @post (postconditions - what is guaranteed AFTER calling)
   // 9. @note/@warning/@see (if applicable)
   // 10. @todo (for formal documentation of possible upcoming changes)
   //
   // You can use @c for code and @p for parameters.
   //
   // IMPORTANT: Do NOT use @throws/@exception tags
   // This codebase uses panic/abort for unrecoverable errors (like precondition
   // violations) rather than C++ exceptions. Precondition violations should be
   // documented with @pre tags. Panics are not exceptions - they terminate the
   // program and are not catchable/recoverable.

   /**
    * @brief Computes a bar value by applying a factor to a base value.
    *
    * @details
    * This function performs a computation using the provided factor and base value.
    * The function panics if the factor is negative, exceeds the maximum safe value,
    * or if the base is zero. The computation is optimized for positive integers.
    *
    * @tparam ResultType The type to cast the result to (must be numeric).
    * @param factor The factor to use in the computation.
    * @param base The base value to multiply with the factor.
    * @pre @p factor must be non-negative.
    * @pre @p factor must be less than @c MAX_SAFE_FACTOR.
    * @pre @p base must not be zero.
    * @return The computed bar value cast to @c ResultType.
    * @post The returned value is always positive.
    * @post The returned value is less than @c MAX_BAR_VALUE.
    * @note This function is thread-safe.
    * @warning This function may lose precision when casting to smaller numeric types.
    * @see @c compute_baz() for a related computation.
    * @see @c apply_factor() for a simpler version without base parameter.
    * @todo Handle MAX_SAFE_FACTOR more elegantly
    */
    template <typename ResultType>
    ResultType compute_bar(int factor, int base);
};
```

### Internal Helpers and Skipping

A tier 2 helper carries a `@brief`, and a `@details` only when the reason for the code
is not obvious from the signature:

```c++
namespace {

/**
 * @brief Trims surrounding whitespace from a raw CSV cell.
 */
std::string trim_cell(const std::string &cell);

/**
 * @brief Returns the number of palettes usable for tile assignment.
 *
 * @details
 * Primary tilesets reserve the first palette for the player sprite, so the usable
 * count is one less than the hardware maximum. Secondary tilesets have no such reservation.
 */
int usable_palette_count(TilesetKind kind);

} // namespace
```

A tier 3 helper gets nothing. If the body reads as plainly as any `@brief` would,
a doc comment is just noise:

```c++
namespace {

// No doc comment needed: the body says it all.
bool is_new_directory(const ManagedPath &path) {
    // An internal comment like this is still permitted if it's genuinely useful
    return path.is_directory() && path.is_new();
}

} // namespace
```

The same "would just restate the signature" test retires doc blocks on trivial public
accessors and mutators, even though they are part of the API. Never write a `@brief`
that restates the function name:

```c++
// CORRECT: no doc needed, the signature says it all
[[nodiscard]] int my_val() const { return my_val_; }

// WRONG: tautological doc block
/**
 * @brief Gets the value of my_val.
 */
[[nodiscard]] int my_val() const { return my_val_; }
```

---

## Test Style

```c++
// Test names: PascalCase, concise. Describe WHAT is tested, not expected behavior.
// Good:
TEST(Rgba32Tests, ToJascStr)
TEST(Rgba32Tests, EqualityIgnoresAlpha)
TEST(ColorSetTests, UnionOfDisjointSets)

// Bad ("ShouldVerb" pattern, sentence-length names):
TEST(Rgba32Tests, ToJascStrShouldWork)
TEST(Rgba32Tests, DefaultConstructedValueShouldBeTransparent)
TEST(Rgba32Tests, OperatorEqualsAndEqualsIgnoringAlphaShouldDifferBasedOnAlpha)

// No section banners (// ===) in test files. The TEST fixture name groups tests.
// No Arrange/Act/Assert labels. Whitespace between setup, action, and assertion is enough.
// No obvious-context comments (e.g., "// Duplicate color"). Comment only when WHY is non-obvious.
```

---

## Idioms and Patterns

### Container Membership Checks

Since we target C++23, prefer `contains` over the `find`/`end` iterator pattern for
associative containers (`std::map`, `std::unordered_map`, `std::set`, etc.):

```c++
// CORRECT — C++20/23 style:
if (my_map.contains(key)) {
    const auto &val = my_map.at(key);
    // ...
}

// AVOID — pre-C++20 style:
if (auto it = my_map.find(key); it != my_map.end()) {
    const auto &val = it->second;
    // ...
}
```

**Exception**: If you need the iterator for purposes beyond just accessing the value
(e.g., erasing, modifying in-place, or iterating from that position), the `find` pattern
is still appropriate.

### Bounds-Checked Element Access

Prefer `.at()` over `operator[]` for element access on `std::vector`, `std::array`,
`std::deque`, and other sequence containers:

```c++
// CORRECT — bounds-checked, throws std::out_of_range on OOB:
const auto &val = my_vec.at(i);
my_array.at(hw) = some_value;

// AVOID — undefined behavior on OOB (silent corruption, crashes, nonsensical values):
const auto &val = my_vec[i];
my_array[hw] = some_value;
```

`operator[]` is UB on out-of-bounds access, which can silently corrupt memory or produce
nonsensical values rather than crashing at the point of error. `.at()` gives a clear
`std::out_of_range` exception, making bugs immediately visible and debuggable.

**Exception**: `operator[]` is fine for `std::map`/`std::unordered_map` when you
intentionally want insertion-on-missing-key semantics.

### std::formatter Specializations

When adding `std::formatter<T>` specializations for custom types, the `format()` method
**MUST** use `auto &ctx` — not `std::format_context &ctx`:

```c++
// CORRECT — works with std::formattable concept on all compilers:
auto format(const MyType &val, auto &ctx) const

// BROKEN on Apple Clang / libc++ — std::formattable<MyType, char> evaluates to false:
auto format(const MyType &val, std::format_context &ctx) const
```

This is due to a libc++ implementation issue (LLVM #66466) where the std::formattable concept
tests against an internal context type, not std::format_context directly.

All formatters should delegate to the type's porytiles::to_string() overload for consistency:

```c++
template <>
struct std::formatter<porytiles::MyType> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles::MyType &value, auto &ctx) const
    {
        return std::format_to(ctx.out(), "{}", porytiles::to_string(value));
    }
};
```

---

## Error and Diagnostic Message Style

All user-facing error and diagnostic messages (in `FormattableError`, `PT_TRY_ASSIGN_CHAIN_ERR`,
`diag_->warning()`, `diag_->error()`, etc.) must follow these rules:

1. **Capital first letter** — the message must start with an uppercase letter
2. **Ends with a period `.`** — every message must end with a period
3. **Single quotes and `Style::bold`** around highlightable items (file names, tileset names, keys):
   `FormatParam{tileset_name, Style::bold}` with `'{}'` in the format string
4. **List headers ending with `:`** are acceptable (e.g. `"To resolve:"`, `"Changes present in Porymap assets:"`)
5. **Empty strings** used as separators are fine
6. **Bullet-point sub-items** in multi-line error lists follow their own style

```c++
// CORRECT:
FormattableError{"Tileset '{}' does not exist.", FormatParam{tileset_name, Style::bold}};
FormattableError{"Failed to read metatile_attributes.bin."};
FormattableError{"Failed to open file for writing: '{}'.", FormatParam{path, Style::bold}};

// WRONG — lowercase start:
FormattableError{"tileset save failed"};

// WRONG — missing period:
FormattableError{"Failed to read metatile_attributes.bin"};

// WRONG — raw string concatenation instead of FormatParam:
FormattableError{"failed to pack palettes for tileset " + tileset_.name()};
// Should be:
FormattableError{"Failed to pack palettes for tileset '{}'.", FormatParam{tileset_.name(), Style::bold}};
```

---

## Prose and Markdown

### Prose Style

The porytiles-user-docs `quickstart.md` page is the tone exemplar for user-facing docs:
confident and clear, it addresses the reader as "you", uses cross-references, and avoids
clichés and tech-jargon-hype language.

Documentation and multi-line doc comments should use [semantic linebreaks](https://sembr.org/):
one sentence or independent clause per line. This applies to Markdown documentation,
README sections, and multi-line Doxygen `@details` blocks.

### Markdown Code Blocks

When writing multi-line C++ code blocks in Markdown, use `c++` after the opening
triple backticks (not `cpp` or `C++`):

```c++
int main() {
    // This is a multiline c++ code block
    // Notice that the triple backticks are followed by "c++"
    return 0;
}
```
