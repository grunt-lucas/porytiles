CXX ?= g++

CXXFLAGS := -Wall -Wpedantic -Werror -std=c++17 -O2 -DPNG_SKIP_SETJMP_CHECK
CXXFLAGS += $(shell pkg-config --cflags libpng)
CXXFLAGS += -Ipng++-0.2.9 -Iinclude

LIBS = -lpng -lz
LDFLAGS += $(shell pkg-config --libs-only-L libpng)

SRCS := src/main.cpp src/png_checks.cpp src/comparators.cpp src/cli_parser.cpp src/palette.cpp src/tile.cpp src/tileset.cpp

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

.PHONY: all clean

all: tscreate$(EXE)
	@:

tscreate$(EXE): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@ $(LDFLAGS) $(LIBS)

clean:
	$(RM) tscreate tscreate.exe
