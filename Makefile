CXX ?= g++

CXXFLAGS := -Wall -std=c++11 -O2

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
	$(CXX) $(CXXFLAGS) $(SRCS) -o $@ $(LDFLAGS)

clean:
	$(RM) tscreate tscreate.exe
