#include "Renderer.h"

bool Renderer::initialize()
{
    // Temporary test geometry; reusable primitive meshes will replace this cube.
    const GLfloat v[] = {
        -1,-1,1, 1,-1,1, 1,1,1, -1,-1,1, 1,1,1, -1,1,1,
        1,-1,-1, -1,-1,-1, -1,1,-1, 1,-1,-1, -1,1,-1, 1,1,-1,
        -1,-1,-1, -1,-1,1, -1,1,1, -1,-1,-1, -1,1,1, -1,1,-1,
        1,-1,1, 1,-1,-1, 1,1,-1, 1,-1,1, 1,1,-1, 1,1,1,
        -1,1,1, 1,1,1, 1,1,-1, -1,1,1, 1,1,-1, -1,1,-1,
        -1,-1,-1, 1,-1,-1, 1,-1,1, -1,-1,-1, 1,-1,1, -1,-1,1
    };
    // Upload the temporary cube once for reuse across frames.
    glGenBuffers(1, &cube_);
    if (!cube_) return false;
    glBindBuffer(GL_ARRAY_BUFFER, cube_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    return true;
}

void Renderer::drawCube() const
{
    // Temporary face colors make the test cube's orientation visible.
    glBindBuffer(GL_ARRAY_BUFFER, cube_);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, nullptr);
    const GLfloat colors[][3] = {
        {0.9f, 0.2f, 0.2f}, {0.2f, 0.7f, 0.9f},
        {0.2f, 0.8f, 0.3f}, {0.9f, 0.7f, 0.2f},
        {0.7f, 0.3f, 0.9f}, {0.9f, 0.5f, 0.2f}
    };
    for (int face = 0; face < 6; ++face) {
        glColor3fv(colors[face]);
        glDrawArrays(GL_TRIANGLES, face * 6, 6);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::shutdown()
{
    // Release the temporary cube while the context is still current.
    if (cube_) glDeleteBuffers(1, &cube_);
    cube_ = 0;
}
