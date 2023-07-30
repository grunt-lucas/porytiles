CXX ?= clang++

# TODO : have an option somewhere to define DOCTEST_CONFIG_DISABLE to remove test code from binary

### Use this to detect if CXX is Clang or GCC ###
COMPILER_VERSION     := $(shell $(CXX) --version)


### Define the targets, source, and build folders ###
SRC                  := src
INCLUDE              := include
BUILD                := build
BIN                  := bin

RELEASE              := release
DEBUG                := debug

PROGRAM              := porytiles
TARGET               := $(PROGRAM)
TEST_TARGET          := $(PROGRAM)-tests

RELEASE_TARGET       := $(RELEASE)/$(BIN)/$(TARGET)
RELEASE_TEST_TARGET  := $(RELEASE)/$(BIN)/$(TEST_TARGET)
DEBUG_TARGET         := $(DEBUG)/$(BIN)/$(TARGET)
DEBUG_TEST_TARGET    := $(DEBUG)/$(BIN)/$(TEST_TARGET)

RELEASE_BUILD        := $(RELEASE)/$(BUILD)
DEBUG_BUILD          := $(DEBUG)/$(BUILD)
SRC_FILES             = $(shell find $(SRC) -type f -name *.cpp)
INCLUDE_FILES         = $(shell find $(INCLUDE) -type f -name *.h)
MAIN_OBJ             := main.o
TESTS_OBJ            := tests.o
RELEASE_OBJ_FILES     = $(filter-out $(RELEASE_BUILD)/$(MAIN_OBJ) $(RELEASE_BUILD)/$(TESTS_OBJ), $(patsubst $(SRC)/%, $(RELEASE_BUILD)/%, $(SRC_FILES:.cpp=.o)))
DEBUG_OBJ_FILES       = $(filter-out $(DEBUG_BUILD)/$(MAIN_OBJ) $(DEBUG_BUILD)/$(TESTS_OBJ), $(patsubst $(SRC)/%, $(DEBUG_BUILD)/%, $(SRC_FILES:.cpp=.o)))


### Compiler and linker flags ###
ifneq '' '$(findstring clang,$(COMPILER_VERSION))'
    CXXFLAGS_COVERAGE := -fprofile-instr-generate -fcoverage-mapping
    LDFLAGS_COVERAGE  := --coverage
else
    CXXFLAGS_COVERAGE :=
    LDFLAGS_COVERAGE  :=
endif

# TODO : include -Wextra, broken right now due to issue in png++ lib
CXXFLAGS             += -Wall -Wpedantic -Werror -std=c++20 -DPNG_SKIP_SETJMP_CHECK
CXXFLAGS             += -I$(INCLUDE) $(shell pkg-config --cflags libpng) -Idoctest-2.4.11 -Ipng++-0.2.9 -Ifmt-10.0.0/include
ifneq '' '$(findstring GCC,$(COMPILER_VERSION))'
# If GCC, do O1, G++13 O2+ is broken when compiling libfmt code, see `potential-gcc-bug' branch
	CXXFLAGS_RELEASE     := $(CXXFLAGS) -O1
else
	CXXFLAGS_RELEASE     := $(CXXFLAGS) -O3
endif
CXXFLAGS_DEBUG       := $(CXXFLAGS) -O0 -g $(CXXFLAGS_COVERAGE)

LDFLAGS              += $(shell pkg-config --libs-only-L libpng) -lpng -lz
LDFLAGS_RELEASE      := $(LDFLAGS)
LDFLAGS_DEBUG        := $(LDFLAGS) $(LDFLAGS_COVERAGE)


### Build rules ###
$(RELEASE_TARGET): $(RELEASE_OBJ_FILES) $(RELEASE_BUILD)/$(MAIN_OBJ)
	@echo "Release: linking ($(CXX)) target..."
	@mkdir -p $(RELEASE)/$(BIN)
	@$(CXX) $^ -o $(RELEASE_TARGET) $(LDFLAGS_RELEASE)

$(RELEASE_TEST_TARGET): $(RELEASE_OBJ_FILES) $(RELEASE_BUILD)/$(TESTS_OBJ)
	@echo "Release: linking ($(CXX)) tests..."
	@mkdir -p $(RELEASE)/$(BIN)
	@$(CXX) $^ -o $(RELEASE_TEST_TARGET) $(LDFLAGS_RELEASE)

$(RELEASE_BUILD)/%.o: $(SRC)/%.cpp
	@echo "Release: compiling ($(CXX)) $<..."
	@mkdir -p $(RELEASE_BUILD)
	@$(CXX) $(CXXFLAGS_RELEASE) -c -o $@ $<

$(DEBUG_TARGET): $(DEBUG_OBJ_FILES) $(DEBUG_BUILD)/$(MAIN_OBJ)
	@echo "Debug: linking ($(CXX)) target..."
	@mkdir -p $(DEBUG)/$(BIN)
	@$(CXX) $^ -o $(DEBUG_TARGET) $(LDFLAGS_DEBUG)

$(DEBUG_TEST_TARGET): $(DEBUG_OBJ_FILES) $(DEBUG_BUILD)/$(TESTS_OBJ)
	@echo "Debug: linking ($(CXX)) tests..."
	@mkdir -p $(DEBUG)/$(BIN)
	@$(CXX) $^ -o $(DEBUG_TEST_TARGET) $(LDFLAGS_DEBUG)

$(DEBUG_BUILD)/%.o: $(SRC)/%.cpp
	@echo "Debug: compiling ($(CXX)) $<..."
	@mkdir -p $(DEBUG_BUILD)
	@$(CXX) $(CXXFLAGS_DEBUG) -c -o $@ $<

.PHONY: clean release debug all check-verbose release-check release-check-verbose debug-check debug-check-verbose

clean:
	$(RM) -r $(RELEASE) $(DEBUG)
	$(RM) default.profraw testcov.profdata

format:
	clang-format -style=file -i $(SRC_FILES) $(INCLUDE_FILES)

release: $(RELEASE_TARGET) $(RELEASE_TEST_TARGET)
	@:

debug: $(DEBUG_TARGET) $(DEBUG_TEST_TARGET)
	@:

all: release debug
	@:

all-check: check debug-check
	@:

check: release
	@./$(RELEASE_TEST_TARGET)

check-verbose: release
	@./$(RELEASE_TEST_TARGET) -s

release-check: check
	# Just an alias for check, for consistency
	@:

release-check-verbose: check-verbose
	# Just an alias for check, for consistency
	@:

debug-check: debug
	@./$(DEBUG_TEST_TARGET)

debug-check-verbose: debug
	@./$(DEBUG_TEST_TARGET) -s
