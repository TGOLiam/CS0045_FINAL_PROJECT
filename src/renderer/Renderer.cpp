#include "Renderer.h"

bool Renderer::initialize()
{
    // Temporary test geometry; reusable primitive meshes will replace this cube.
    const std::vector<GLfloat> v = {
        -1,-1,1, 1,-1,1, 1,1,1, -1,-1,1, 1,1,1, -1,1,1,
        1,-1,-1, -1,-1,-1, -1,1,-1, 1,-1,-1, -1,1,-1, 1,1,-1,
        -1,-1,-1, -1,-1,1, -1,1,1, -1,-1,-1, -1,1,1, -1,1,-1,
        1,-1,1, 1,-1,-1, 1,1,-1, 1,-1,1, 1,1,-1, 1,1,1,
        -1,1,1, 1,1,1, 1,1,-1, -1,1,1, 1,1,-1, -1,1,-1,
        -1,-1,-1, 1,-1,-1, 1,-1,1, -1,-1,-1, 1,-1,1, -1,-1,1
    };
    // Upload the temporary cube once for reuse across frames.
    if (!cube_.upload(v, GL_TRIANGLES)) return false;
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    return true;
}

void Renderer::drawCube() const
{
    // Temporary face colors make the test cube's orientation visible.
    const GLfloat colors[][3] = {
        {0.9f, 0.2f, 0.2f}, {0.2f, 0.7f, 0.9f},
        {0.2f, 0.8f, 0.3f}, {0.9f, 0.7f, 0.2f},
        {0.7f, 0.3f, 0.9f}, {0.9f, 0.5f, 0.2f}
    };
    for (int face = 0; face < 6; ++face) {
        glColor3fv(colors[face]);
        cube_.drawRange(face * 6, 6);
    }
}

void Renderer::shutdown()
{
    // Release the temporary cube while the context is still current.
    cube_.release();
}
