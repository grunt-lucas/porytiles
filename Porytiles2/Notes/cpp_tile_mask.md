# C++ Tile Mask System Implementation Outline

## Overview
This document outlines the C++ implementation of the tile mask system from the Rust borytiles codebase. The system handles tile shape representation, flipping, and canonical orientation finding.

**Target**: C++23
**Error Handling**: Assertions (no exceptions)
**Hash Functions**: Simple implementations provided

## Required Headers

```c++
#include <array>
#include <bitset>
#include <map>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cassert>
```

## Utility Functions

### Bit Reversal
```c++
// Simple bit reversal for uint8_t (used in horizontal flipping)
inline constexpr uint8_t reverse_bits(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}
```

## Core Data Structures

### 1. `MaskPixels`
**Purpose**: Represents which pixels in an 8x8 tile are non-transparent (1 bit per pixel).

```c++
struct MaskPixels {
    std::array<uint8_t, 8> rows = {0};

    constexpr MaskPixels() = default;

    // Get flipped version
    MaskPixels get_flip(bool h, bool v) const {
        if (!h && !v) return *this;

        MaskPixels result;
        int8_t v_inc = v ? -1 : 1;
        int8_t v_start = v ? 7 : 0;

        for (int y = 0; y < 8; ++y) {
            int ry = y * v_inc + v_start;
            result.rows[y] = h ? reverse_bits(rows[ry]) : rows[ry];
        }
        return result;
    }

    // Comparison operators (lexicographic on rows array)
    auto operator<=>(const MaskPixels&) const = default;
};

// Hash specialization for use in unordered containers
namespace std {
    template<>
    struct hash<MaskPixels> {
        size_t operator()(const MaskPixels& tm) const {
            // Simple hash combining all 8 bytes
            size_t h = 0;
            for (uint8_t byte : tm.rows) {
                h = h * 31 + byte;
            }
            return h;
        }
    };
}
```

**Implementation notes:**
- `get_flip()`: Early return if no flipping needed; vertical flip iterates rows in reverse order; horizontal flip reverses bits in each byte
- C++23 spaceship operator `<=>` provides all comparisons automatically (lexicographic on array)

---

### 2. `ColorIndex`
**Purpose**: Wrapper around palette color index (0-255).

```c++
struct ColorIndex {
    uint8_t value = 0;

    constexpr ColorIndex() = default;
    constexpr explicit ColorIndex(uint8_t v) : value(v) {}

    auto operator<=>(const ColorIndex&) const = default;
};
```

---

### 3. `ColorSet`
**Purpose**: 256-bit bitset for tracking which color indices are used. Supports up to 256 colors.

```c++
struct ColorSet {
    std::bitset<256> bits;

    ColorSet() = default;

    // Set operations
    ColorSet union_with(const ColorSet& other) const {
        ColorSet result;
        result.bits = bits | other.bits;
        return result;
    }

    ColorSet intersection(const ColorSet& other) const {
        ColorSet result;
        result.bits = bits & other.bits;
        return result;
    }

    // Count set bits
    int16_t size() const {
        return static_cast<int16_t>(bits.count());
    }

    // Subset/superset checks
    bool is_superset_of(const ColorSet& other) const {
        return (bits & other.bits) == other.bits;
    }

    bool is_subset_of(const ColorSet& other) const {
        return (bits & other.bits) == bits;
    }

    // Bit manipulation
    void set_bit(ColorIndex idx) {
        bits.set(idx.value);
    }

    void unset_bit(ColorIndex idx) {
        bits.reset(idx.value);
    }

    bool get_bit(ColorIndex idx) const {
        return bits.test(idx.value);
    }

    // Comparison
    auto operator<=>(const ColorSet&) const = default;
};

namespace std {
    template<>
    struct hash<ColorSet> {
        size_t operator()(const ColorSet& ics) const {
            return std::hash<std::bitset<256>>{}(ics.bits);
        }
    };
}
```

**Implementation notes:**
- Uses `std::bitset<256>` for automatic bit management
- Much simpler than manual array management
- `count()` is already optimized for bit counting
- Subset check: `(a & b) == b` means all bits in `b` are also in `a`

---

### 4. `MaskTile`
**Purpose**: Maps tile masks to color indices, defining the tile's visual appearance.

```c++
struct MaskTile {
    std::map<MaskPixels, ColorIndex> colors;

    MaskTile() = default;

    // Get flipped version (flips all masks, preserves color mappings)
    MaskTile get_flip(bool h, bool v) const {
        if (!h && !v) return *this;

        MaskTile result;
        for (const auto& [mask, color] : colors) {
            result.colors[mask.get_flip(h, v)] = color;
        }
        return result;
    }

    // Find canonical orientation (minimum among 4 flips)
    IsoCanonicalTile get_ideal_flip() const {
        std::array<std::pair<bool, bool>, 4> flips = {{
            {false, false},
            {false, true},
            {true, false},
            {true, true}
        }};

        std::vector<IsoCanonicalTile> candidates;
        candidates.reserve(4);

        for (const auto& [h, v] : flips) {
            candidates.emplace_back(get_flip(h, v), h, v);
        }

        return *std::min_element(candidates.begin(), candidates.end());
    }

    // Get palette index that contains all colors in this tile
    uint8_t get_palette(const std::vector<ColorSet>& palettes) const {
        ColorSet tile_colors;
        for (const auto& [mask, color_idx] : colors) {
            tile_colors.set_bit(color_idx);
        }

        for (size_t p = 0; p < palettes.size(); ++p) {
            if (palettes[p].is_superset_of(tile_colors)) {
                return static_cast<uint8_t>(p);
            }
        }

        assert(false && "No palette contains all tile colors");
        return 0;
    }

    // CRITICAL: Custom comparison that ONLY compares keys, not values
    // This is the key to canonical orientation finding - it compares ONLY the shape masks, not the colors
    bool operator==(const MaskTile& other) const = default;

    bool operator<(const MaskTile& other) const {
        std::vector<MaskPixels> a, b;
        a.reserve(colors.size());
        b.reserve(other.colors.size());

        for (const auto& [key, _] : colors) {
            a.push_back(key);
        }
        for (const auto& [key, _] : other.colors) {
            b.push_back(key);
        }

        return a < b;  // Lexicographic vector comparison
    }
};

namespace std {
    template<>
    struct hash<MaskTile> {
        size_t operator()(const MaskTile& tile) const {
            // Hash only the keys, not the values
            size_t h = 0;
            for (const auto& [key, _] : tile.colors) {
                h = h * 31 + std::hash<MaskPixels>{}(key);
            }
            return h;
        }
    };
}
```

**Critical implementation detail:**
- `operator<` must extract keys into a vector, then compare lexicographically - this is KEY to canonical flip finding
- `operator==` uses default map equality (compares both keys AND values)
- Hash should only hash the keys (matching Rust implementation)
- `get_palette()` finds which palette can represent all colors in this tile

---

### 5. `IsoCanonicalTile`
**Purpose**: Stores a canonical tile shape with the flip flags needed to reconstruct the original.

```c++
struct IsoCanonicalTile {
    MaskTile shape;
    bool flip_h = false;
    bool flip_v = false;

    IsoCanonicalTile() = default;

    IsoCanonicalTile(
        const MaskTile& s,
        bool h,
        bool v
    ) : shape(s), flip_h(h), flip_v(v) {}

    // Standard comparison (compares all fields in order)
    auto operator<=>(const IsoCanonicalTile& other) const = default;
};

namespace std {
    template<>
    struct hash<IsoCanonicalTile> {
        size_t operator()(const IsoCanonicalTile& tii) const {
            size_t h = std::hash<MaskTile>{}(tii.shape);
            h = h * 31 + std::hash<bool>{}(tii.flip_h);
            h = h * 31 + std::hash<bool>{}(tii.flip_v);
            return h;
        }
    };
}
```

**Implementation note:**
- Spaceship operator provides tuple-like comparison: first `shape`, then `flip_h`, then `flip_v`
- This enables `std::min_element()` in `get_ideal_flip()` to work correctly

---

## Example Usage

### Creating a Tile and Finding Its Canonical Form

```c++
// Create a tile with some pattern
MaskTile tile;

// Add a mask for color 5 (e.g., a diagonal line)
MaskPixels mask1;
mask1.rows = {0b10000000, 0b01000000, 0b00100000, 0b00010000,
              0b00001000, 0b00000100, 0b00000010, 0b00000001};
tile.colors[mask1] = ColorIndex(5);

// Add another mask for color 3 (e.g., corners)
MaskPixels mask2;
mask2.rows = {0b10000001, 0b00000000, 0b00000000, 0b00000000,
              0b00000000, 0b00000000, 0b00000000, 0b10000001};
tile.colors[mask2] = ColorIndex(3);

// Find the canonical (minimal) orientation
IsoCanonicalTile canonical = tile.get_ideal_flip();

// canonical.shape contains the canonicalized tile
// canonical.flip_h and canonical.flip_v tell you how to transform it back
std::cout << "Canonical form requires: "
          << (canonical.flip_h ? "H-flip " : "")
          << (canonical.flip_v ? "V-flip " : "")
          << std::endl;
```

### Using ColorSet for Palette Matching

```c++
// Create palettes
std::vector<ColorSet> palettes(4);

// Palette 0 contains colors 0-15
for (uint8_t i = 0; i < 16; ++i) {
    palettes[0].set_bit(ColorIndex(i));
}

// Palette 1 contains colors 16-31
for (uint8_t i = 16; i < 32; ++i) {
    palettes[1].set_bit(ColorIndex(i));
}

// Find which palette can be used for this tile
uint8_t palette_idx = canonical.shape.get_palette(palettes);
std::cout << "Tile uses palette: " << (int)palette_idx << std::endl;
```

### Comparing Tiles

```c++
MaskTile tile_a, tile_b;
// ... populate tiles ...

// Equality compares BOTH shape AND colors
if (tile_a == tile_b) {
    std::cout << "Tiles are identical" << std::endl;
}

// Less-than compares ONLY the shape (keys), not colors
if (tile_a < tile_b) {
    std::cout << "Tile A has lexicographically smaller shape" << std::endl;
}
```

---

## Understanding the Custom Comparison

The most important concept in this system is the **custom `operator<` for `MaskTile`**:

```c++
bool operator<(const MaskTile& other) const {
    // Extract ONLY the keys (MaskPixels values), ignore the colors
    std::vector<MaskPixels> a, b;
    for (const auto& [key, _] : colors) a.push_back(key);
    for (const auto& [key, _] : other.colors) b.push_back(key);
    return a < b;
}
```

### Why This Matters

When `get_ideal_flip()` calls `std::min_element()` on the 4 flip variants:
1. It compares them using `operator<`
2. This comparison **ignores color values** and only looks at the **shape pattern** (which masks exist)
3. The "minimum" tile is the one with the lexicographically smallest set of masks
4. This ensures the same canonical form regardless of which colors are used

### Contrast with `operator==`

```c++
bool operator==(const MaskTile& other) const = default;
```

- Equality uses **default map comparison** which checks both keys AND values
- Two tiles are equal only if they have the same masks AND the same color assignments
- This is correct because tiles with different colors are truly different tiles

### Example

```
Tile A: { mask1 → color5, mask2 → color3 }
Tile B: { mask1 → color7, mask2 → color9 }
```

- `A == B` is **false** (different colors)
- `A < B` compares only {mask1, mask2} vs {mask1, mask2}, so neither is less (they're equivalent in ordering)
- Both would canonicalize to the same shape orientation

---

## Build Order Recommendation

1. **Utility functions**: `reverse_bits()` helper
2. **ColorIndex**: Simple wrapper (no dependencies)
3. **ColorSet**: 256-bit bitset with set operations
4. **MaskPixels**: 8x8 bit pattern with flip operations
5. **MaskTile**: Map-based tile representation with custom comparison
6. **IsoCanonicalTile**: Final class that ties it all together

---

## Additional Considerations

- **Performance**: std::map lookups are O(log n). If you need faster lookups and don't need ordering, consider `std::unordered_map` with custom hash.
- **Memory**: Current design mirrors Rust. If memory is tight, could optimize the map representation.
- **Thread safety**: None of these structures are thread-safe. Add mutexes if needed for concurrent access.

---

## Summary

This tile mask system provides a sophisticated way to:

1. **Represent tiles** as sets of bit patterns (masks) mapped to color indices
2. **Transform tiles** through horizontal and vertical flipping
3. **Canonicalize tiles** by finding the lexicographically minimal flip orientation
4. **Deduplicate tiles** that are identical under flipping
5. **Match tiles to palettes** using efficient bitset operations

### Key Implementation Points

- Use C++23's spaceship operator `<=>` for automatic comparison generation
- `MaskPixels` uses simple bit manipulation for 8x8 pixel patterns
- `ColorSet` uses `std::bitset<256>` for clean, efficient set operations
- `MaskTile` has **two different comparison semantics**:
  - `operator==`: compares shape AND colors (full equality)
  - `operator<`: compares shape ONLY (for canonicalization)
- `get_ideal_flip()` leverages the custom `operator<` to find minimal orientation
- All hash functions hash only the semantically relevant data
- Assertions are used for error handling (no exceptions)

The entire system is designed to be efficient, type-safe, and mirror the Rust implementation's semantics while taking advantage of modern C++ features.
