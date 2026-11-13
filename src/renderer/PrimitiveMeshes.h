#pragma once

#include <vector>

namespace PrimitiveMeshes {
// Cube is centered at the origin with side length one; each face has six vertices.
std::vector<float> cube();
// Unit line along X, a point at the origin, and a unit quad in the XY plane.
std::vector<float> line();
std::vector<float> point();
std::vector<float> quad();
}
