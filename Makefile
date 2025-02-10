SHELL = /bin/bash

CXX         = g++
EXECUTABLE  = main

# If main() is in a different file, update this:
PROJECTFILE = main.cpp

CXXFLAGS    = -std=c++17 -Wconversion -Wall -Werror -Wextra -pedantic

TESTSOURCES = $(wildcard test*.cpp)
TESTSOURCES := $(filter-out $(PROJECTFILE), $(TESTSOURCES))

SOURCES     = $(wildcard *.cpp)
SOURCES     := $(filter-out $(TESTSOURCES), $(SOURCES))
OBJECTS     = $(SOURCES:%.cpp=%.o)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(EXECUTABLE)

# Debug build -> creates main_debug
debug: CXXFLAGS += -g3 -DDEBUG -fsanitize=address -fsanitize=undefined -D_GLIBCXX_DEBUG
debug:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o main_debug

# Release build -> creates main
release: CXXFLAGS += -O3 -DNDEBUG
release: $(EXECUTABLE)

# Profile build -> creates main_profile
profile: CXXFLAGS += -g3
profile:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o main_profile

static:
	cppcheck --enable=all --suppress=missingIncludeSystem $(SOURCES) *.h *.hpp
.PHONY: static

all: debug release profile
.PHONY: all

# Test driver support
TESTS = $(TESTSOURCES:%.cpp=%)

define make_tests
  SRCS = $(filter-out $(PROJECTFILE), $(SOURCES))
  OBJS = $(SRCS:%.cpp=%.o)
  $(1): CXXFLAGS += -g3 -DDEBUG
  $(1): $(OBJS) $(1).cpp
	$(CXX) $(CXXFLAGS) $(OBJS) $(1).cpp -o $(1)
endef
$(foreach test, $(TESTS), $(eval $(call make_tests,$(test))))

alltests: $(TESTS)
.PHONY: alltests

clean:
	rm -Rf *.dSYM
	rm -f $(OBJECTS) $(EXECUTABLE) \
	      main_debug \
	      main_profile \
	      $(TESTS) perf.data*
.PHONY: clean
