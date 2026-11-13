#pragma once

#include "Mesh.h"

class Renderer {
public:
    bool initialize();
    // Temporary Stage 1 test cube.
    void drawCube() const;
    void shutdown();
private:
    // Temporary mesh for the test cube.
    Mesh cube_;
};
