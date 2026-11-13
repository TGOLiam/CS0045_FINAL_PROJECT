CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic
LDLIBS ?= -lGLEW -lglut -lGL -lGLU

TARGET := build/helicopter-simulator
SOURCES := src/main.cpp src/application/Application.cpp src/input/Input.cpp src/renderer/Renderer.cpp src/renderer/Mesh.cpp src/renderer/PrimitiveMeshes.cpp src/camera/Camera.cpp src/math/Matrix4.cpp

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(SOURCES) src/application/Application.h src/application/AppState.h src/input/Input.h src/renderer/Renderer.h src/renderer/Mesh.h src/renderer/PrimitiveMeshes.h src/renderer/Color.h src/camera/Camera.h src/math/Matrix4.h
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -rf build
