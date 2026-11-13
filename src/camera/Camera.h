#pragma once
#include "../math/Matrix4.h"

class Camera {
public:
    // Updating the target preserves the current orbit and distance.
    void setTarget(Vec3 target);
    void orbit(float yawDeltaDegrees, float pitchDeltaDegrees);
    void zoom(float distanceDelta);
    Matrix4 viewMatrix() const;
    Matrix4 projectionMatrix(int width, int height) const;

private:
    // Initial orbit reproduces the earlier view from approximately (4, 3, 5).
    Vec3 target_ = {0.0f, 0.0f, 0.0f};
    float yawDegrees_ = 38.6598f;
    float pitchDegrees_ = 25.1041f;
    float distance_ = 7.0711f;
    static constexpr float minPitch_ = -80.0f;
    static constexpr float maxPitch_ = 80.0f;
    static constexpr float minDistance_ = 3.0f;
    static constexpr float maxDistance_ = 40.0f;
};
