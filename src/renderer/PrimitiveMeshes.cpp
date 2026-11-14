#include "PrimitiveMeshes.h"
#include <cmath>

std::vector<float> PrimitiveMeshes::cube()
{
    return {
        -.5f,-.5f,.5f, .5f,-.5f,.5f, .5f,.5f,.5f, -.5f,-.5f,.5f, .5f,.5f,.5f, -.5f,.5f,.5f,
        .5f,-.5f,-.5f, -.5f,-.5f,-.5f, -.5f,.5f,-.5f, .5f,-.5f,-.5f, -.5f,.5f,-.5f, .5f,.5f,-.5f,
        -.5f,-.5f,-.5f, -.5f,-.5f,.5f, -.5f,.5f,.5f, -.5f,-.5f,-.5f, -.5f,.5f,.5f, -.5f,.5f,-.5f,
        .5f,-.5f,.5f, .5f,-.5f,-.5f, .5f,.5f,-.5f, .5f,-.5f,.5f, .5f,.5f,-.5f, .5f,.5f,.5f,
        -.5f,.5f,.5f, .5f,.5f,.5f, .5f,.5f,-.5f, -.5f,.5f,.5f, .5f,.5f,-.5f, -.5f,.5f,-.5f,
        -.5f,-.5f,-.5f, .5f,-.5f,-.5f, .5f,-.5f,.5f, -.5f,-.5f,-.5f, .5f,-.5f,.5f, -.5f,-.5f,.5f
    };
}

std::vector<float> PrimitiveMeshes::line() { return {-.5f, 0, 0, .5f, 0, 0}; }
std::vector<float> PrimitiveMeshes::point() { return {0, 0, 0}; }
std::vector<float> PrimitiveMeshes::quad()
{
    return {-.5f,-.5f,0, .5f,-.5f,0, .5f,.5f,0,
            -.5f,-.5f,0, .5f,.5f,0, -.5f,.5f,0};
}

std::vector<float> PrimitiveMeshes::disc()
{
    // A unit-diameter disc in the XY plane, made once from 24 triangles.
    constexpr int segments = 24;
    constexpr float pi = 3.14159265358979323846f;
    std::vector<float> vertices;
    for (int i = 0; i < segments; ++i) {
        const float a = 2.0f * pi * i / segments;
        const float b = 2.0f * pi * (i + 1) / segments;
        vertices.insert(vertices.end(), {0.0f, 0.0f, 0.0f,
            0.5f * std::cos(a), 0.5f * std::sin(a), 0.0f,
            0.5f * std::cos(b), 0.5f * std::sin(b), 0.0f});
    }
    return vertices;
}

std::vector<float> PrimitiveMeshes::cone()
{
    constexpr int segments = 8;
    constexpr float pi = 3.14159265358979323846f;
    std::vector<float> vertices;
    for (int i = 0; i < segments; ++i) {
        const float a = 2.0f * pi * i / segments;
        const float b = 2.0f * pi * (i + 1) / segments;
        const float ax = 0.5f * std::cos(a);
        const float az = 0.5f * std::sin(a);
        const float bx = 0.5f * std::cos(b);
        const float bz = 0.5f * std::sin(b);
        vertices.insert(vertices.end(), {ax, 0.0f, az, 0.0f, 1.0f, 0.0f, bx, 0.0f, bz,
                                         0.0f, 0.0f, 0.0f, ax, 0.0f, az, bx, 0.0f, bz});
    }
    return vertices;
}
