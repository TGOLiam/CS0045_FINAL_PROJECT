CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2
PYTHON = python

ifeq ($(OS),Windows_NT)

LDLIBS = -lglew32 -lfreeglut -lopengl32 -lglu32
TARGET = build/helicopter-simulator.exe
SOURCES = $(wildcard src/*.cpp) $(wildcard src/*/*.cpp)
OBJECTS = $(patsubst src/%.cpp,build/%.o,$(SOURCES))
DEPENDENCIES = $(OBJECTS:.o=.d)

.PHONY: all clean city-asset

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@if not exist "$(@D)" mkdir "$(@D)"
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@

build/%.o: src/%.cpp
	@if not exist "$(@D)" mkdir "$(@D)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDENCIES)

city-asset:
	$(PYTHON) tools/generate_city.py

clean:
	@if exist build rmdir /s /q build

else

LDLIBS = -lGLEW -lglut -lGL -lGLU
TARGET = build/helicopter-simulator
SOURCES = $(shell find src -type f -name '*.cpp')
OBJECTS = $(patsubst src/%.cpp,build/%.o,$(SOURCES))
DEPENDENCIES = $(OBJECTS:.o=.d)

.PHONY: all clean city-asset

all: $(TARGET)

$(TARGET): $(OBJECTS)
	mkdir -p $(@D)
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@

build/%.o: src/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDENCIES)

city-asset:
	$(PYTHON) tools/generate_city.py

clean:
	rm -rf build

endif
