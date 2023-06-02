CXX ?= g++

CXXFLAGS := -Wall -Werror -std=c++17 -O2 -DPNG_SKIP_SETJMP_CHECK
CXXFLAGS += $(shell pkg-config --cflags libpng)
CXXFLAGS += -Ipng++-0.2.9 -Iinclude

LIBS = -lpng -lz
LDFLAGS += $(shell pkg-config --libs-only-L libpng)

SRCS := src/tscreate.cpp src/init_checks.cpp

HEADERS := include/tscreate.h include/tscreate.h

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

.PHONY: all clean

all: tscreate$(EXE)
	@:

tscreate$(EXE): $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@ $(LDFLAGS) $(LIBS)

clean:
	$(RM) tscreate tscreate.exe
