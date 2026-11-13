#include "Renderer.h"
#include "PrimitiveMeshes.h"
#include <stdexcept>

bool Renderer::initialize()
{
    // Upload static primitives once; release earlier uploads if any upload fails.
    if (!cube_.upload(PrimitiveMeshes::cube(), GL_TRIANGLES) ||
        !line_.upload(PrimitiveMeshes::line(), GL_LINES) ||
        !point_.upload(PrimitiveMeshes::point(), GL_POINTS) ||
        !quad_.upload(PrimitiveMeshes::quad(), GL_TRIANGLES)) {
        shutdown();
        return false;
    }
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

void Renderer::begin3D(const Matrix4& view, const Matrix4& projection)
{
    if (interfaceActive_) throw std::logic_error("End interface pass before starting 3D");
    glEnable(GL_DEPTH_TEST);
    setViewProjection(view, projection);
}

void Renderer::beginInterface(int width, int height)
{
    if (interfaceActive_ || width <= 0 || height <= 0)
        throw std::invalid_argument("Invalid interface pass");
    savedView_ = view_;
    savedProjection_ = projection_;
    savedDepthTest_ = glIsEnabled(GL_DEPTH_TEST);
    glGetIntegerv(GL_MATRIX_MODE, &savedMatrixMode_);
    // Preserve the actual OpenGL matrices as well as Renderer's cached matrices.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glDisable(GL_DEPTH_TEST);
    setViewProjection(Matrix4::identity(),
                      Matrix4::orthographic(0, static_cast<float>(width),
                                            static_cast<float>(height), 0, -1, 1));
    interfaceActive_ = true;
}

void Renderer::endInterface()
{
    if (!interfaceActive_) throw std::logic_error("No interface pass is active");
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(static_cast<GLenum>(savedMatrixMode_));
    if (savedDepthTest_) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
    view_ = savedView_;
    projection_ = savedProjection_;
    interfaceActive_ = false;
}

const Mesh& Renderer::meshFor(MeshId mesh) const
{
    switch (mesh) {
    case MeshId::Cube: return cube_;
    case MeshId::Line: return line_;
    case MeshId::Point: return point_;
    case MeshId::Quad: return quad_;
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
    // Balance the matrix stacks if shutdown occurs during an interface pass.
    if (interfaceActive_) endInterface();
    cube_.release();
    line_.release();
    point_.release();
    quad_.release();
}
