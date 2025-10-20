# Chafa Terminal Graphics Library - Implementation Notes

## Overview

Chafa is a terminal graphics library (LGPLv3+) that converts images into various terminal-compatible formats. This document explains how it works internally and how to integrate it into projects.

---

## How Chafa Prints Color Images to Terminals

Chafa supports multiple methods for displaying color images in terminals, from most compatible to most advanced:

### 1. ANSI Colored Blocks (Most Compatible)

This is the simplest method that works in almost any terminal.

**Escape Sequences** (from `chafa/chafa-term-db.c:373-375`):
```
Foreground: "\033[38;2;R;G;Bm"  // Set text color
Background: "\033[48;2;R;G;Bm"  // Set background color
```

**Basic Algorithm** (from `chafa/internal/chafa-canvas-printer.c:218-252`):
1. For each cell position in the output grid:
   - Emit foreground color: `ESC[38;2;R;G;Bm`
   - Emit background color: `ESC[48;2;R;G;Bm`
   - Output a Unicode block character (like █ U+2588 or half-blocks)
2. Use ANSI reset: `ESC[0m` to reset colors

**Simple Example**:
```c++
// To print a red pixel followed by a blue pixel:
printf("\033[38;2;255;0;0m█");     // Red foreground + full block
printf("\033[38;2;0;0;255m█");     // Blue foreground + full block
printf("\033[0m\n");                 // Reset
```

### 2. Sixel Graphics Protocol

A more advanced bitmap protocol from the 1980s, still widely supported.

**Format** (from `chafa/internal/chafa-sixel-canvas.c:498-521`):
```
ESC P [params] q        # Begin sixels (DCS - Device Control String)
"1;1;WIDTH;HEIGHT       # Raster attributes
#0;2;R;G;B              # Define color palette (R,G,B are 0-100 range)
#1;2;R;G;B              # More palette entries...
#0 [sixel data]         # Select color #0 and output sixels
-                       # GNL (Graphics Newline) - move to next row
...
ESC \                   # End sixels (ST - String Terminator)
```

**Sixel Data Encoding**:
- Each character represents 6 vertical pixels
- Characters '?' (0x3F) through '~' (0x7E) encode bit patterns
- Character '?' = `000000` (no pixels), 'A' = `000001` (bottom pixel only), etc.

### 3. Kitty Graphics Protocol

Modern protocol with the best quality (from `chafa/internal/chafa-kitty-canvas.c:299-304`):
```
ESC _G a=T,f=32,s=WIDTH,v=HEIGHT,c=COLS,r=ROWS,m=1 ESC \
[base64-encoded RGBA pixel data in chunks]
ESC _G m=0 ESC \
```

### 4. iTerm2 Inline Images

iTerm2-specific protocol (from `chafa/chafa-term-db.c:443-444`):
```
ESC ]1337;File=inline=1;width=W;height=H;preserveAspectRatio=0:[base64 data] BEL
```

---

## Minimal Implementation Example

Here's a minimal example for the ANSI colored blocks method:

```c++
#include <stdio.h>

void print_rgb_pixel(int r, int g, int b) {
    // Set foreground to RGB color and print full block
    printf("\033[38;2;%d;%d;%dm█", r, g, b);
}

void reset_colors() {
    printf("\033[0m");
}

int main() {
    // Print a simple gradient
    for (int i = 0; i < 256; i++) {
        print_rgb_pixel(i, 0, 255 - i);  // Red to blue gradient
    }
    reset_colors();
    printf("\n");

    return 0;
}
```

---

## Key Escape Sequence Breakdown

- `\033` or `\x1b` = ESC character
- `[` = CSI (Control Sequence Introducer)
- `38;2;R;G;B` = Set foreground to RGB (24-bit color)
- `48;2;R;G;B` = Set background to RGB (24-bit color)
- `m` = SGR (Select Graphic Rendition) terminator

---

## Licensing & Integration

### License

Chafa is licensed under **LGPLv3+** (GNU Lesser General Public License v3.0 or later).

Both the library and frontend tools are covered by this license.

### Integration Options for MIT-Licensed Projects

The LGPL was specifically designed to allow proprietary/MIT software to use LGPL libraries. You have several options:

#### Option 1: Dynamic Linking (Recommended & Easiest)

Ship a **separate** Chafa shared library (`.so`, `.dylib`, or `.dll`) alongside your MIT-licensed executable.

**Advantages:**
- ✅ Cleanest separation
- ✅ Users can upgrade/replace Chafa independently
- ✅ Minimal LGPL obligations

**CMake approach:**
```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(CHAFA REQUIRED chafa>=1.0)

target_link_libraries(your_app PRIVATE ${CHAFA_LIBRARIES})
target_include_directories(your_app PRIVATE ${CHAFA_INCLUDE_DIRS})
```

**Requirements:**
- Include the LGPL license text with your distribution
- Add a notice that your software uses Chafa
- Don't modify Chafa itself (or if you do, publish those changes)

#### Option 2: Static Linking (More Complex)

You CAN statically link Chafa, but LGPLv3 Section 4d requires you to allow users to replace the library:

**Option 4d(0)** - Provide relinking capability:
- Distribute your object files (`.o`) OR source code
- Provide build instructions so users can relink with a modified Chafa

**Option 4d(1)** - Use a shared library mechanism (same as option 1)

**CMake with FetchContent** (static):
```cmake
include(FetchContent)
FetchContent_Declare(
    chafa
    GIT_REPOSITORY https://github.com/hpjansson/chafa.git
    GIT_TAG        master  # or a specific version tag
)
FetchContent_MakeAvailable(chafa)
target_link_libraries(your_app PRIVATE chafa)
```

**Additional Requirements for Static Linking:**
- Everything from option 1, PLUS
- Provide a way for users to relink with their own Chafa version
- Most commonly: make your project open source OR provide object files

#### Option 3: Don't Use Chafa - Roll Your Own

Implement just the basic ANSI escape sequence method yourself. This would be:
- ✅ No licensing concerns
- ✅ Smaller binary
- ✅ Complete control
- ⚠️ More work upfront

### Example Attribution

In your README:
```markdown
## Third-Party Libraries

This software uses the Chafa library (https://hpjansson.org/chafa/)
Licensed under GNU Lesser General Public License v3.0 or later
See COPYING.LESSER for the full license text
```

---

## Runtime Dynamic Loading (Best Option)

Load the Chafa library at runtime using `dlopen()` (Linux/macOS) or `LoadLibrary()` (Windows). This way your binary works everywhere, and gracefully upgrades functionality when Chafa is present.

### Benefits

- ✅ **Single binary** works everywhere
- ✅ Users don't need to recompile to get Chafa benefits
- ✅ "Install Chafa, restart app" - instant upgrade
- ✅ No build-time dependency on Chafa
- ✅ Easy to distribute
- ✅ Fallback to simple implementation if Chafa not available

### Example Implementation

```c++
// chafa_loader.h
#pragma once
#include <memory>
#include <string>

class ChafaLoader {
public:
    ChafaLoader();
    ~ChafaLoader();

    bool isAvailable() const { return loaded_; }

    // Print image using Chafa if available, fallback otherwise
    void printImage(const uint8_t* rgba_data, int width, int height);

private:
    void* handle_ = nullptr;
    bool loaded_ = false;

    // Function pointers to Chafa API
    void* (*chafa_canvas_new_)(void*) = nullptr;
    void (*chafa_canvas_draw_all_pixels_)(void*, int, const uint8_t*, int, int, int) = nullptr;
    void* (*chafa_canvas_print_)(void*, void*) = nullptr;
    void (*chafa_canvas_unref_)(void*) = nullptr;
    // ... more function pointers as needed

    void loadChafa();
    void fallbackPrint(const uint8_t* rgba_data, int width, int height);
};
```

```c++
// chafa_loader.cpp
#include "chafa_loader.h"
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
    #define DLOPEN(name) LoadLibraryA(name)
    #define DLSYM(handle, name) GetProcAddress((HMODULE)handle, name)
    #define DLCLOSE(handle) FreeLibrary((HMODULE)handle)
#else
    #include <dlfcn.h>
    #define DLOPEN(name) dlopen(name, RTLD_LAZY | RTLD_LOCAL)
    #define DLSYM(handle, name) dlsym(handle, name)
    #define DLCLOSE(handle) dlclose(handle)
#endif

ChafaLoader::ChafaLoader() {
    loadChafa();
}

ChafaLoader::~ChafaLoader() {
    if (handle_) {
        DLCLOSE(handle_);
    }
}

void ChafaLoader::loadChafa() {
    // Try to load the Chafa shared library
    const char* library_names[] = {
#ifdef _WIN32
        "chafa.dll",
        "libchafa.dll",
#elif __APPLE__
        "libchafa.dylib",
        "libchafa.0.dylib",
        "/usr/local/lib/libchafa.dylib",
        "/opt/homebrew/lib/libchafa.dylib",
#else
        "libchafa.so",
        "libchafa.so.0",
        "/usr/lib/libchafa.so",
        "/usr/local/lib/libchafa.so",
#endif
        nullptr
    };

    for (int i = 0; library_names[i] != nullptr; i++) {
        handle_ = DLOPEN(library_names[i]);
        if (handle_) {
            std::cout << "✓ Found Chafa library: " << library_names[i] << std::endl;
            break;
        }
    }

    if (!handle_) {
        std::cout << "ℹ Chafa not found, using fallback renderer" << std::endl;
        return;
    }

    // Load function pointers
    chafa_canvas_new_ = (void*(*)(void*))DLSYM(handle_, "chafa_canvas_new");
    chafa_canvas_draw_all_pixels_ = (void(*)(void*, int, const uint8_t*, int, int, int))
        DLSYM(handle_, "chafa_canvas_draw_all_pixels");
    chafa_canvas_print_ = (void*(*)(void*, void*))DLSYM(handle_, "chafa_canvas_print");
    chafa_canvas_unref_ = (void(*)(void*))DLSYM(handle_, "chafa_canvas_unref");

    // Verify all critical functions loaded
    if (chafa_canvas_new_ && chafa_canvas_draw_all_pixels_ &&
        chafa_canvas_print_ && chafa_canvas_unref_) {
        loaded_ = true;
        std::cout << "✓ Chafa functions loaded successfully" << std::endl;
    } else {
        std::cout << "⚠ Chafa library found but incomplete API, using fallback" << std::endl;
        DLCLOSE(handle_);
        handle_ = nullptr;
    }
}

void ChafaLoader::printImage(const uint8_t* rgba_data, int width, int height) {
    if (loaded_) {
        // Use Chafa (simplified - you'd need proper config setup)
        std::cout << "Using Chafa renderer..." << std::endl;
        // ... actual Chafa API calls using function pointers
    } else {
        fallbackPrint(rgba_data, width, height);
    }
}

void ChafaLoader::fallbackPrint(const uint8_t* rgba_data, int width, int height) {
    std::cout << "Using simple ANSI renderer..." << std::endl;

    // Simple ANSI escape sequence implementation
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;
            uint8_t r = rgba_data[idx];
            uint8_t g = rgba_data[idx + 1];
            uint8_t b = rgba_data[idx + 2];

            // Print colored block
            printf("\033[38;2;%d;%d;%dm█", r, g, b);
        }
        printf("\033[0m\n");
    }
}
```

### Usage Example

```bash
# Without Chafa installed
$ ./myapp image.png
ℹ Chafa not found, using fallback renderer
[simple colored blocks appear]

# After user installs Chafa
$ sudo apt install libchafa0  # or brew install chafa
$ ./myapp image.png
✓ Found Chafa library: libchafa.so.0
✓ Chafa functions loaded successfully
Using Chafa renderer...
[high quality output appears]
```

### Optional: Add a Help Message

```c++
void printHelp() {
    std::cout << "MyApp v1.0\n\n";
    std::cout << "Image rendering: ";

    ChafaLoader loader;
    if (loader.isAvailable()) {
        std::cout << "Enhanced (Chafa) ✓\n";
    } else {
        std::cout << "Basic (fallback)\n";
        std::cout << "\nTip: Install Chafa for better image quality:\n";
        std::cout << "  Ubuntu/Debian: sudo apt install libchafa0\n";
        std::cout << "  macOS:         brew install chafa\n";
        std::cout << "  Arch:          sudo pacman -S chafa\n";
    }
}
```

---

## Chafa Codebase Structure

### Key Files

- **`chafa/chafa-canvas.c/h`**: Main canvas for rendering images
- **`chafa/chafa-placement.c/h`**: Handles different image placement protocols
- **`chafa/chafa-term-seq-def.h`**: Defines terminal escape sequences
- **`chafa/internal/chafa-canvas-printer.c`**: Generates ANSI sequences for symbol/character mode
- **`chafa/internal/chafa-sixel-canvas.c`**: Sixel protocol implementation
- **`chafa/internal/chafa-kitty-canvas.c`**: Kitty graphics protocol implementation
- **`chafa/internal/chafa-iterm2-canvas.c`**: iTerm2 protocol implementation
- **`chafa/chafa-term-info.c`**: Terminal sequence formatting and emission
- **`chafa/chafa-term-db.c`**: Terminal capability detection and database

### API Flow

1. Create a `ChafaCanvas` with configuration
2. Call `chafa_canvas_draw_all_pixels()` with pixel data
3. Call `chafa_canvas_print()` to generate terminal escape sequences
4. Output the resulting string to the terminal

---

## Resources

- **Official Website**: https://hpjansson.org/chafa/
- **API Documentation**: https://hpjansson.org/chafa/ref/
- **GitHub Repository**: https://github.com/hpjansson/chafa
- **License**: LGPLv3+ (Lesser GPL version 3 or later)

---

## Recommendations

1. **For simple use cases**: Implement basic ANSI escape sequence method yourself (100-200 lines)
2. **For full features**: Use runtime dynamic loading to optionally use Chafa
3. **For distribution**: Ship with fallback implementation, let users opt into Chafa
4. **For licensing**: Dynamic linking is the cleanest approach with MIT projects

This gives you zero dependencies for casual users, enhanced features for power users, all from a single binary!
