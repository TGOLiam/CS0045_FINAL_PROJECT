#pragma once

#include "Mesh.h"
#include "Color.h"
#include "../math/Matrix4.h"
#include <memory>
#include <vector>

enum class MeshId { Cube, Line, Point, Quad, Disc };

class Renderer {
public:
    // Initialize and shut down while the OpenGL context is current.
    bool initialize();
    void beginFrame(int width, int height, Color clearColor);
    void setViewProjection(const Matrix4& view, const Matrix4& projection);
    // Camera supplies view and projection; scene objects supply model and color.
    void begin3D(const Matrix4& view, const Matrix4& projection);
    void drawSkybox(Vec3 cameraPosition);
    // Interface coordinates have their origin at the top-left of the window.
    void beginInterface(int width, int height);
    void endInterface();
    // Model transforms a unit mesh into world space; each draw is independent.
    void draw(MeshId mesh, const Matrix4& model, Color color) const;
    // Renderer owns uploaded static meshes until shutdown.
    bool uploadStaticMesh(const std::vector<float>& positions, GLenum primitive, std::size_t& id);
    void drawStaticMesh(std::size_t id, const Matrix4& model, Color color) const;
    // Temporary face-range drawing supports the test cube's six colors.
    // Replace its use in Application with normal scene-object draws.
    void drawRange(MeshId mesh, const Matrix4& model, Color color, GLint first, GLsizei count) const;
    void shutdown();
private:
    Mesh cube_;
    Mesh line_;
    Mesh point_;
    Mesh quad_;
    Mesh disc_;
    std::vector<std::unique_ptr<Mesh>> staticMeshes_;
    Matrix4 view_ = Matrix4::identity();
    Matrix4 projection_ = Matrix4::identity();
    Matrix4 savedView_ = Matrix4::identity();
    Matrix4 savedProjection_ = Matrix4::identity();
    GLboolean savedDepthTest_ = GL_FALSE;
    GLint savedMatrixMode_ = GL_MODELVIEW;
    bool interfaceActive_ = false;
    const Mesh& meshFor(MeshId mesh) const;
    void applyTransformAndColor(const Matrix4& model, Color color) const;
};
