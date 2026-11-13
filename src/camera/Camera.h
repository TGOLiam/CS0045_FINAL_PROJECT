#pragma once
#include "../math/Matrix4.h"

class Camera {
public:
    void setTarget(Vec3 target);
    Matrix4 viewMatrix() const;
    Matrix4 projectionMatrix(int width, int height) const;

private:
    // Initial orbit reproduces the earlier view from approximately (4, 3, 5).
    Vec3 target_ = {0.0f, 0.0f, 0.0f};
    float yawDegrees_ = 38.6598f;
    float pitchDegrees_ = 25.1041f;
    float distance_ = 7.0711f;
};
