CXX ?= g++
#CXX = /opt/homebrew/bin/g++-13

# TODO : set optimization to O2 and remove debug for final build
CXXFLAGS   := -Wall -Wpedantic -Werror -std=c++17 -O0 -DPNG_SKIP_SETJMP_CHECK -g
CXXFLAGS   += $(shell pkg-config --cflags libpng)
CXXFLAGS   += -Idoctest-2.4.11 -Ipng++-0.2.9 -Iinclude
SRCDIR      = src
BUILDDIR    = build
SRCS        = $(shell find $(SRCDIR) -type f -name *.cpp)
OBJS        = $(filter-out $(BUILDDIR)/main.o $(BUILDDIR)/tests.o, $(patsubst $(SRCDIR)/%, $(BUILDDIR)/%, $(SRCS:.cpp=.o)))
LDFLAGS    += $(shell pkg-config --libs-only-L libpng) -lpng -lz
ifeq ($(OS),Windows_NT)
EXE        := .exe
else
EXE        :=
endif
PROGRAM     = porytiles
TARGET      = $(PROGRAM)$(EXE)
TEST_TARGET = $(PROGRAM)-tests$(EXE)

$(TARGET): $(OBJS) $(BUILDDIR)/main.o
	@echo "Linking ($(CXX)) $@..."
	@$(CXX) $^ -o $(TARGET) $(LDFLAGS)

$(TEST_TARGET): $(OBJS) $(BUILDDIR)/tests.o
	@echo "Linking ($(CXX)) $@..."
	@$(CXX) $^ -o $(TEST_TARGET) $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(BUILDDIR)
	@echo "Compiling ($(CXX)) $<..."
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY: all check clean
all: clean $(TARGET) $(TEST_TARGET)
	@:

check: $(TEST_TARGET)
	@echo "Running tests..."
	@./$(TEST_TARGET)

clean:
	$(RM) $(TARGET) $(TEST_TARGET)
	$(RM) -r $(BUILDDIR)
	$(RM) -r $(PROGRAM).dSYM
