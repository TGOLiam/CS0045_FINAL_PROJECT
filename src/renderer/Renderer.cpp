#include "Renderer.h"
#include <stdexcept>

bool Renderer::initialize()
{
    // Temporary test geometry; move the cube definition to PrimitiveMeshes.
    const std::vector<GLfloat> v = {
        -1,-1,1, 1,-1,1, 1,1,1, -1,-1,1, 1,1,1, -1,1,1,
        1,-1,-1, -1,-1,-1, -1,1,-1, 1,-1,-1, -1,1,-1, 1,1,-1,
        -1,-1,-1, -1,-1,1, -1,1,1, -1,-1,-1, -1,1,1, -1,1,-1,
        1,-1,1, 1,-1,-1, 1,1,-1, 1,-1,1, 1,1,-1, 1,1,1,
        -1,1,1, 1,1,1, 1,1,-1, -1,1,1, 1,1,-1, -1,1,-1,
        -1,-1,-1, 1,-1,-1, 1,-1,1, -1,-1,-1, 1,-1,1, -1,-1,1
    };
    // Temporary cube upload; initialize the reusable primitive collection here later.
    if (!cube_.upload(v, GL_TRIANGLES)) return false;
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    return true;
}

void Renderer::beginFrame(int width, int height, Color clearColor)
{
    glViewport(0, 0, width, height);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::setViewProjection(const Matrix4& view, const Matrix4& projection)
{
    view_ = view;
    projection_ = projection;
}

const Mesh& Renderer::meshFor(MeshId mesh) const
{
    // Temporary lookup; extend it to return meshes from the primitive collection.
    switch (mesh) {
    case MeshId::Cube: return cube_;
    }
    throw std::invalid_argument("Unknown mesh");
}

void Renderer::applyTransformAndColor(const Matrix4& model, Color color) const
{
    // Load complete matrices for each object to keep transforms independent.
    const Matrix4 modelView = view_ * model;
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection_.data());
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(modelView.data());
    glColor4f(color.r, color.g, color.b, color.a);
}

void Renderer::draw(MeshId mesh, const Matrix4& model, Color color) const
{
    applyTransformAndColor(model, color);
    meshFor(mesh).draw();
}

void Renderer::drawRange(MeshId mesh, const Matrix4& model, Color color, GLint first, GLsizei count) const
{
    // Temporary cube faces use this range; scene objects can use draw().
    applyTransformAndColor(model, color);
    meshFor(mesh).drawRange(first, count);
}

void Renderer::shutdown()
{
    // Release the temporary cube; later release all owned primitive meshes here.
    cube_.release();
}
