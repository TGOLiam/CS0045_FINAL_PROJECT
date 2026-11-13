#include "PrimitiveMeshes.h"

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
