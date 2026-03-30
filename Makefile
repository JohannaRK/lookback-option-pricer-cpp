# ------------------------------------------------------------------------------
# Makefile – Lookback Option Monte Carlo
# ------------------------------------------------------------------------------
# - Uses Homebrew LLVM clang++ on macOS if available
# - Falls back to default compiler on Linux/other systems
# - Builds a single executable: lookback
# ------------------------------------------------------------------------------

# Detect OS
UNAME_S := $(shell uname -s)

# Compiler selection
ifeq ($(UNAME_S),Darwin)
CXX ?= /opt/homebrew/opt/llvm/bin/clang++
else
CXX ?= c++
endif

# Compilation flags
CXXFLAGS = -std=c++17 -O3 -Iinclude -Wall -Wextra
OPTFLAGS = -ffast-math -funroll-loops
OMPFLAGS = -fopenmp
ARCHFLAGS = -march=native

# Linker flags
LDFLAGS = $(OMPFLAGS) $(ARCHFLAGS)

# Sources
SRC_ALL = $(filter-out src/LookbackOption.cpp, $(wildcard src/*.cpp))

# App (CLI) sources/objects
APP_SRC = $(SRC_ALL)
APP_OBJ = $(APP_SRC:.cpp=.o)

# Shared library sources/objects (no main.cpp)
LIB_SRC = $(filter-out src/main.cpp, $(SRC_ALL))
LIB_OBJ = $(LIB_SRC:.cpp=.pic.o)

# Targets
TARGET = lookback
LIBTARGET = liblookback.so

# ------------------------------------------------------------------------------
# Default target
# ------------------------------------------------------------------------------
all: $(TARGET)

# ------------------------------------------------------------------------------
# Object compilation
# ------------------------------------------------------------------------------

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $(OMPFLAGS) $(ARCHFLAGS) -c $< -o $@

# Position-independent objects for shared library
%.pic.o: %.cpp
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $(OMPFLAGS) $(ARCHFLAGS) -fPIC -c $< -o $@

# ------------------------------------------------------------------------------
# Link executable
# ------------------------------------------------------------------------------

$(TARGET): $(APP_OBJ)
	$(CXX) $^ -o $@ $(LDFLAGS)

# Build a Linux shared library (useful for testing; Excel on Windows needs a .dll)
$(LIBTARGET): $(LIB_OBJ)
	$(CXX) -shared $^ -o $@ $(LDFLAGS)

# ------------------------------------------------------------------------------
# Clean
# ------------------------------------------------------------------------------
clean:
	rm -f src/*.o src/*.pic.o $(TARGET) $(LIBTARGET)

.PHONY: all clean