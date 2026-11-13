#pragma once

#include "Mesh.h"
#include "Color.h"
#include "../math/Matrix4.h"

// Temporary cube identifier; extend this when reusable primitives are added.
enum class MeshId { Cube };

class Renderer {
public:
    bool initialize();
    void beginFrame(int width, int height, Color clearColor);
    void setViewProjection(const Matrix4& view, const Matrix4& projection);
    void draw(MeshId mesh, const Matrix4& model, Color color) const;
    // Temporary face-range drawing supports the test cube's six colors.
    // Replace its use in Application with normal scene-object draws.
    void drawRange(MeshId mesh, const Matrix4& model, Color color, GLint first, GLsizei count) const;
    void shutdown();
private:
    // Temporary cube mesh; replace with the reusable primitive mesh collection.
    Mesh cube_;
    Matrix4 view_ = Matrix4::identity();
    Matrix4 projection_ = Matrix4::identity();
    const Mesh& meshFor(MeshId mesh) const;
    void applyTransformAndColor(const Matrix4& model, Color color) const;
};
