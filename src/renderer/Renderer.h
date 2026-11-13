#pragma once

#include <GL/glew.h>

class Renderer {
public:
    bool initialize();
    // Temporary Stage 1 test cube.
    void drawCube() const;
    void shutdown();
private:
    // Temporary VBO for the test cube.
    GLuint cube_ = 0;
};
