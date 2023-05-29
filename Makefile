CXX ?= g++

CXXFLAGS := -Wall -Werror -std=c++11 -O2 -DPNG_SKIP_SETJMP_CHECK
CXXFLAGS += $(shell pkg-config --cflags libpng)
CXXFLAGS += -Icxxopts

LIBS = -lpng -lz
LDFLAGS += $(shell pkg-config --libs-only-L libpng)

SRCS := tscreate.cpp

HEADERS := tscreate.h

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

.PHONY: all clean

all: tscreate$(EXE)
	@:

mapjson$(EXE): $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@ $(LDFLAGS) $(LIBS)

clean:
	$(RM) tscreate tscreate.exe
