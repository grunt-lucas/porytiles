CXX ?= clang++

# TODO : set optimization to O2 and remove debug for final build
# TODO : this makefile is broken with gcc due to the CXXCOV option here

CXXCOV      = -fprofile-instr-generate -fcoverage-mapping
CXXFLAGS    = -Wall -Wpedantic -Werror -std=c++17 -O0 -DPNG_SKIP_SETJMP_CHECK -g $(CXXCOV)
CXXFLAGS   += $(shell pkg-config --cflags libpng)
CXXFLAGS   += -Idoctest-2.4.11 -Ipng++-0.2.9 -Iinclude
SRCDIR      = src
BUILDDIR    = build
SRCS        = $(shell find $(SRCDIR) -type f -name *.cpp)
MAINOBJ     = main.o
TESTSOBJ    = tests.o
OBJS        = $(filter-out $(BUILDDIR)/$(MAINOBJ) $(BUILDDIR)/$(TESTSOBJ), $(patsubst $(SRCDIR)/%, $(BUILDDIR)/%, $(SRCS:.cpp=.o)))
LDCOV       = --coverage
LDFLAGS    += $(shell pkg-config --libs-only-L libpng) -lpng -lz $(LDCOV)
ifeq ($(OS),Windows_NT)
EXE        := .exe
else
EXE        :=
endif
PROGRAM     = porytiles
TARGET      = $(PROGRAM)$(EXE)
TEST_TARGET = $(PROGRAM)-tests$(EXE)

$(TARGET): $(OBJS) $(BUILDDIR)/$(MAINOBJ)
	@echo "Linking ($(CXX)) $@..."
	@$(CXX) $^ -o $(TARGET) $(LDFLAGS)

$(TEST_TARGET): $(OBJS) $(BUILDDIR)/$(TESTSOBJ)
	@echo "Linking ($(CXX)) $@..."
	@$(CXX) $^ -o $(TEST_TARGET) $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(BUILDDIR)
	@echo "Compiling ($(CXX)) $<..."
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

# If the first argument is "coverage-show"...
ifeq (coverage-show,$(firstword $(MAKECMDGOALS)))
  # use the rest as arguments for "coverage-show"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif
# If the first argument is "coverage-report"...
ifeq (coverage-report,$(firstword $(MAKECMDGOALS)))
  # use the rest as arguments for "coverage-report"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif

ifeq ($(strip $(RUN_ARGS)),)
$(error arguments must be supplied to coverage command)
endif

.PHONY: clean all target tests check

clean:
	$(RM) $(TARGET) $(TEST_TARGET)
	$(RM) -r $(BUILDDIR)
	$(RM) -r $(PROGRAM).dSYM
	$(RM) default.profraw testcov.profdata

all: $(TARGET) $(TEST_TARGET)
	@:

target: $(TARGET)
	@:

tests: $(TEST_TARGET)

check: tests
	@./$(TEST_TARGET)

coverage-show: check
	@xcrun llvm-profdata merge -o testcov.profdata default.profraw
	@xcrun llvm-cov show ./$(TEST_TARGET) -instr-profile=testcov.profdata $(RUN_ARGS)

coverage-report: check
	@xcrun llvm-profdata merge -o testcov.profdata default.profraw
	@xcrun llvm-cov report ./$(TEST_TARGET) -instr-profile=testcov.profdata $(RUN_ARGS)
