CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic
LDLIBS ?= -lglew32 -lfreeglut -lopengl32 -lglu32

TARGET := build/helicopter-simulator

SOURCES := src/main.cpp src/application/Application.cpp src/input/Input.cpp src/renderer/Renderer.cpp src/renderer/Mesh.cpp src/renderer/PrimitiveMeshes.cpp src/camera/Camera.cpp src/math/Matrix4.cpp src/customization/CustomizationMenu.cpp src/audio/Audio.cpp src/helicopter/Helicopter.cpp

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(SOURCES) src/application/Application.h src/application/AppState.h src/input/Input.h src/renderer/Renderer.h src/renderer/Mesh.h src/renderer/PrimitiveMeshes.h src/renderer/Color.h src/camera/Camera.h src/math/Matrix4.h src/customization/CustomizationMenu.h src/customization/CustomizationSettings.h src/audio/Audio.h src/helicopter/Helicopter.h
	if not exist "$(@D)" mkdir "$(@D)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

clean:
	if exist build rmdir /s /q build