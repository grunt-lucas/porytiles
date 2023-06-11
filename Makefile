CXX ?= g++

CXXFLAGS := -Wall -Wpedantic -Werror -std=c++17 -O0 -DPNG_SKIP_SETJMP_CHECK -g
CXXFLAGS += $(shell pkg-config --cflags libpng)
CXXFLAGS += -Ipng++-0.2.9 -Iinclude

LIBS = -lpng -lz
LDFLAGS += $(shell pkg-config --libs-only-L libpng)

SRCS = $(shell find src -type f -name *.cpp)

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

.PHONY: all clean

all: porytiles$(EXE)
	@:

porytiles$(EXE): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@ $(LDFLAGS) $(LIBS)

clean:
	$(RM) porytiles porytiles.exe
