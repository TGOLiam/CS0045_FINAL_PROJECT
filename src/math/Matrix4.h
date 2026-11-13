#pragma once

#include <array>

struct Vec3 {
  float x, y, z;
};

// Column-major storage and column vectors, matching OpenGL.
// Element (row, column) is stored at column * 4 + row.
// In A * B * vertex, B acts on the vertex first.
class Matrix4 {
public:
  // Identity leaves a position or direction unchanged.
  static Matrix4 identity();
  // Translation moves positions by (x, y, z).
  static Matrix4 translation(float x, float y, float z);
  // Scale changes each coordinate relative to the origin.
  static Matrix4 scale(float x, float y, float z);
  // Rotate in degrees around an axis through the origin, using the right-hand rule.
  // The axis must have nonzero length.
  static Matrix4 rotation(float degrees, Vec3 axis);
  // Perspective makes distant objects appear smaller; fov is vertical degrees.
  static Matrix4 perspective(float fov, float aspect, float nearPlane,
                             float farPlane);
  // Orthographic projection keeps apparent size independent of distance.
  static Matrix4 orthographic(float left, float right, float bottom, float top,
                              float nearPlane, float farPlane);
  // Build a world-to-camera view from the eye, target, and approximate up direction.
  static Matrix4 lookAt(Vec3 eye, Vec3 target, Vec3 up);
  Matrix4 operator*(const Matrix4 &other) const;
  const float *data() const { return values_.data(); }

private:
  std::array<float, 16> values_{};
};
