#include "Matrix4.h"
#include <cmath>
#include <stdexcept>

namespace {
constexpr float pi = 3.14159265358979323846f;
// Dot product combines corresponding components; it also measures alignment.
float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
// Cross product gives a vector perpendicular to both inputs.
Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
Vec3 normalize(Vec3 v) {
  // Divide by length to keep direction while making length equal to one.
  const float n = std::sqrt(dot(v, v));
  if (n <= 1e-6f)
    throw std::invalid_argument("Zero length vector");
  return {v.x / n, v.y / n, v.z / n};
}
} // namespace
Matrix4 Matrix4::identity() {
  Matrix4 m;
  // The diagonal ones preserve x, y, z, and the homogeneous coordinate w.
  m.values_[0] = m.values_[5] = m.values_[10] = m.values_[15] = 1;
  return m;
}
Matrix4 Matrix4::translation(float x, float y, float z) {
  Matrix4 m = identity();
  // The last column offsets positions (whose homogeneous w is 1).
  m.values_[12] = x;
  m.values_[13] = y;
  m.values_[14] = z;
  return m;
}
Matrix4 Matrix4::scale(float x, float y, float z) {
  Matrix4 m = identity();
  // Each diagonal entry multiplies its corresponding coordinate.
  m.values_[0] = x;
  m.values_[5] = y;
  m.values_[10] = z;
  return m;
}
Matrix4 Matrix4::rotation(float degrees, Vec3 axis) {
  // A unit axis makes rotation independent of the input vector's length.
  axis = normalize(axis);
  const float x = axis.x;
  const float y = axis.y;
  const float z = axis.z;
  // Trigonometric functions take radians. Positive angles follow the right-hand rule.
  const float c = std::cos(degrees * pi / 180);
  const float s = std::sin(degrees * pi / 180);
  const float t = 1 - c;
  Matrix4 m = identity();
  // Rodrigues' rotation formula fills the upper-left 3x3 rotation matrix.
  // Indices increase down columns: 0, 1, 2 are the first column.
  m.values_[0] = t * x * x + c;
  m.values_[4] = t * x * y - s * z;
  m.values_[8] = t * x * z + s * y;
  m.values_[1] = t * x * y + s * z;
  m.values_[5] = t * y * y + c;
  m.values_[9] = t * y * z - s * x;
  m.values_[2] = t * x * z - s * y;
  m.values_[6] = t * y * z + s * x;
  m.values_[10] = t * z * z + c;
  return m;
}
Matrix4 Matrix4::perspective(float fov, float aspect, float n, float f) {
  if (!(fov > 0 && fov < 180 && aspect > 0 && n > 0 && f > n))
    throw std::invalid_argument("Invalid perspective");
  // Half the vertical field of view determines the projection's focal scale.
  const float q = 1 / std::tan(fov * pi / 360);
  Matrix4 m;
  m.values_[0] = q / aspect;
  m.values_[5] = q;
  // These entries map depth and set w = -z for the perspective divide.
  m.values_[10] = (f + n) / (n - f);
  m.values_[11] = -1;
  m.values_[14] = 2 * f * n / (n - f);
  return m;
}
Matrix4 Matrix4::orthographic(float l, float r, float b, float t, float n,
                              float f) {
  if (l == r || b == t || n == f)
    throw std::invalid_argument("Invalid orthographic projection");
  // Scale and shift the specified viewing box into OpenGL's clip space.
  Matrix4 m = identity();
  m.values_[0] = 2 / (r - l);
  m.values_[5] = 2 / (t - b);
  m.values_[10] = -2 / (f - n);
  m.values_[12] = -(r + l) / (r - l);
  m.values_[13] = -(t + b) / (t - b);
  m.values_[14] = -(f + n) / (f - n);
  return m;
}
Matrix4 Matrix4::lookAt(Vec3 eye, Vec3 target, Vec3 up) {
  // Build perpendicular forward, side, and corrected up directions.
  const Vec3 f =
      normalize({target.x - eye.x, target.y - eye.y, target.z - eye.z});
  const Vec3 s = normalize(cross(f, up));
  const Vec3 u = cross(s, f);
  // The basis rotates world coordinates into camera space; dot products
  // translate the eye to the camera-space origin.
  Matrix4 m = identity();
  m.values_[0] = s.x;
  m.values_[4] = s.y;
  m.values_[8] = s.z;
  m.values_[1] = u.x;
  m.values_[5] = u.y;
  m.values_[9] = u.z;
  m.values_[2] = -f.x;
  m.values_[6] = -f.y;
  m.values_[10] = -f.z;
  m.values_[12] = -dot(s, eye);
  m.values_[13] = -dot(u, eye);
  m.values_[14] = dot(f, eye);
  return m;
}
Matrix4 Matrix4::operator*(const Matrix4 &other) const {
  Matrix4 result;
  // Each result entry is a row of this matrix dotted with a column of other.
  for (int col = 0; col < 4; ++col)
    for (int row = 0; row < 4; ++row)
      for (int k = 0; k < 4; ++k)
        result.values_[col * 4 + row] +=
            values_[k * 4 + row] * other.values_[col * 4 + k];
  return result;
}
