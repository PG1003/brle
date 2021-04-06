# This makefile is based on https://gist.github.com/maxtruxa/4b3929e118914ccef057f8a05c614b0f
# ... and mangled by PG1003

SRCDIR  = src
UTILDIR = util
TESTDIR = tests

# source files
SRCS := $(shell find $(UTILDIR) -type f -name '*.cpp')
TESTSRCS := $(shell find $(TESTDIR) -type f -name '*.cpp')

# intermediate directory for generated dependency and object files
OBJDIR := obj

# object files, auto generated from source files
OBJS := $(patsubst %,$(OBJDIR)/%.o,$(basename $(SRCS)))
TESTOBJS := $(patsubst %,$(OBJDIR)/%.o,$(basename $(TESTSRCS)))
# dependency files, auto generated from source files
DEPS := $(patsubst %,$(OBJDIR)/%.d,$(basename $(SRCS)))
TESTDEPS := $(patsubst %,$(OBJDIR)/%.d,$(basename $(TESTSRCS)))

# compilers (at least gcc and clang) don't create the subdirectories automatically
$(shell mkdir -p $(dir $(OBJS)) >/dev/null)
$(shell mkdir -p $(dir $(TESTOBJS)) >/dev/null)

# C++ compiler
CXX := g++
# linker
LD := g++

# C++ flags
CXXFLAGS := -std=c++20
# C/C++ flags
CPPFLAGS := -Wall -Wextra -Wpedantic -O3
# Extra include directories
INCLUDES = -I "./src"
# linker flags
LDFLAGS :=
# linker flags: libraries to link (e.g. -lfoo)
LDLIBS :=
# flags required for dependency generation; passed to compilers
DEPFLAGS = -MT $@ -MD -MP -MF $*.d

# compile C++ source files
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) -c -o $@
# link object files to binary
LINK.o = $(LD) $(LDFLAGS) $(LDLIBS) -o $@

.PHONY: all
all: brle test

.PHONY: clean
clean:
	$(RM) -r $(OBJDIR)

.PHONY: brle
brle: $(OBJDIR)/brle

$(OBJDIR)/brle: $(OBJS)
	$(LINK.o) $^

.PHONY: test
test: $(OBJDIR)/test

$(OBJDIR)/test: $(TESTOBJS)
	$(LINK.o) $^

.PHONY: run_tests
run_tests: test brle
	@echo "Running tests..."
	@echo "> Generic tests"
	@cd $(OBJDIR); ./test && : || { echo ">>> Generic tests failed!"; exit 1; }
	@echo "> brle decode"
	@cd $(OBJDIR); ./brle -d ../$(TESTDIR)/test.bmp.rle test.bmp && : || { echo ">>> brle decode test failed!";  exit 1; }
	@echo "> brle encode"
	@cd $(OBJDIR); ./brle -e test.bmp test.bmp.rle && : || { echo ">>> brle encode test failed!"; exit 1; }
	@echo "> brle decode 2"
	@cd $(OBJDIR); ./brle -d test.bmp.rle test2.bmp && : || { echo ">>> brle decode 2 test failed!";  exit 1; }
	@echo "> brle validate results"
	@cd $(OBJDIR); cmp -s ../$(TESTDIR)/test.bmp.rle test.bmp.rle && : || { echo ">>> brle RLE validation failed!";  exit 1; }
	@cd $(OBJDIR); cmp -s test.bmp test2.bmp && : || { echo ">>> brle data validation failed!";  exit 1; }
	@echo ""
	@echo "...tests completed"
	@echo "      _"
	@echo "     /(|"
	@echo "    (  :"
	@echo "   __\  \  _____"
	@echo " (____)  '|"
	@echo "(____)|   |"
	@echo " (____).__|"
	@echo "  (___)__.|_____"
# https://asciiart.website/index.php?art=people/body%20parts/hand%20gestures


$(OBJS): $(SRCS)
$(OBJS): $(SRCS) $(DEPS)
	$(COMPILE.cc) $<

$(TESTOBJS): $(TESTSRCS)
$(TESTOBJS): $(TESTSRCS) $(TESTDEPS)
	$(COMPILE.cc) $<

.PRECIOUS: $(OBJDIR)/%.d
$(OBJDIR)/%.d: ;

-include $(DEPS) 
