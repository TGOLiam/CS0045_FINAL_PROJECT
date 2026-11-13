CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic
LDLIBS ?= -lGLEW -lglut -lGL -lGLU

TARGET := build/helicopter-simulator
SOURCES := src/main.cpp src/application/Application.cpp

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(SOURCES) src/application/Application.h src/application/AppState.h
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -rf build
