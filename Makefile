CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic
LDLIBS ?= -lGLEW -lglut -lGL -lGLU

TARGET := build/helicopter-simulator
# Discover subsystem sources and keep their directory layout under build/.
SOURCES := $(sort $(shell find src -type f -name '*.cpp'))
OBJECTS := $(patsubst src/%.cpp,build/%.o,$(SOURCES))
DEPENDENCIES := $(OBJECTS:.o=.d)

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJECTS) Makefile
	@mkdir -p $(@D)
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@

# Compiler-generated .d files record the headers used by each source file.
build/%.o: src/%.cpp Makefile
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDENCIES)

clean:
	rm -rf build
