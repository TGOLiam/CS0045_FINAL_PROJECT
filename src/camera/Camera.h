#pragma once
#include "../math/Matrix4.h"

class Camera {
public:
    // Temporary fixed view for the Stage 1 cube.
    Matrix4 viewMatrix() const;
    Matrix4 projectionMatrix(int width, int height) const;
};
