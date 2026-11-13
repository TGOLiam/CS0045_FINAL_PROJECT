#pragma once

#include <GL/glew.h>
#include <vector>

class Mesh {
public:
    Mesh() = default;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Upload xyz positions once. A current OpenGL context is required.
    bool upload(const std::vector<GLfloat>& positions, GLenum primitive);
    void draw() const;
    // Draw a subset of vertices; first and count are vertex indices, not bytes.
    void drawRange(GLint first, GLsizei count) const;
    // Renderer calls this before the OpenGL context is destroyed.
    void release();

private:
    GLuint vbo_ = 0;
    GLenum primitive_ = GL_TRIANGLES;
    GLsizei vertexCount_ = 0;
};
