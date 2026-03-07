# std::formatter Guide for Porytiles2

## The libc++ Bug (LLVM #66466)

Apple Clang 17 / libc++ has a bug where `std::formattable<T, char>` evaluates to `false` for
types whose `std::formatter` specialization uses a concrete `std::format_context &` parameter
instead of `auto &ctx`.

**LLVM Issue:** https://github.com/llvm/llvm-project/issues/66466

### Why It Happens

The `std::formattable` concept internally checks if the formatter can work with an
implementation-specific context type, not `std::format_context` directly. When you write:

```c++
auto format(const MyType &val, std::format_context &ctx) const
```

The concept check fails because the concrete parameter type doesn't match the internal context
type. When you write:

```c++
auto format(const MyType &val, auto &ctx) const
```

The template parameter accepts any context type, so the concept check passes.

### Empirical Verification

Verified on this machine with Apple Clang 17 / libc++:
- `std::formatter<Rgba32>` with `std::format_context &ctx`: `std::formattable<Rgba32, char>` = **false**
- `std::formatter<Rgba32>` with `auto &ctx`: `std::formattable<Rgba32, char>` = **true**

In both cases, `std::format("{}", rgba)` works at runtime — the bug only affects the concept check.

## How This Affects Porytiles2

### FormatParam

`FormatParam::resolve_string()` uses `std::formattable` as its primary dispatch mechanism.
If a type's formatter uses `std::format_context &`, it won't be recognized as formattable and
will fall through to the `operator<<` fallback — still working, but taking a less efficient path.

### LazyLayeredConfig

`LazyLayeredConfig::resolve_config_value()` uses `std::format("{}", resolved_value)` to cache
string representations. This requires all config value types to be formattable. Previously it
used an ADL hack with `using std::to_string; using porytiles2::to_string;` which was fragile
and hard to understand.

### The Relationship

The fix enables a clean pipeline:
1. Custom types provide `porytiles2::to_string()` overloads
2. `std::formatter<T>` specializations delegate to `to_string()`
3. `std::formattable<T, char>` evaluates to `true` (with `auto &ctx`)
4. `FormatParam` and `LazyLayeredConfig` use `std::format()` uniformly

## Pattern for New Types

Every custom type that needs string conversion should follow this pattern:

```c++
// 1. In the type's header, inside namespace porytiles2:
inline std::string to_string(const MyType &val) { /* ... */ }

// 2. After closing namespace porytiles2:
template <>
struct std::formatter<porytiles2::MyType> {
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }

    auto format(const porytiles2::MyType &value, auto &ctx) const  // MUST be auto &ctx
    {
        return std::format_to(ctx.out(), "{}", porytiles2::to_string(value));
    }
};
```

This ensures the type works with:
- `std::format("{}", val)` — direct formatting
- `std::formattable<MyType, char>` — concept checks
- `FormatParam{val}` — error message formatting
- `LazyLayeredConfig` — config value caching
