#pragma once

#include <vector>

namespace PrimitiveMeshes {
// Cube is centered at the origin with side length one; each face has six vertices.
std::vector<float> cube();
// Unit line along X, a point at the origin, and a unit quad in the XY plane.
std::vector<float> line();
std::vector<float> point();
std::vector<float> quad();
std::vector<float> disc();
// Eight-sided cone centered on XZ, with base y=0, tip y=1, diameter 1.
std::vector<float> cone();
// Eight-sided cylinder along Z, centered at the origin, unit diameter and length.
std::vector<float> cylinder();
}
