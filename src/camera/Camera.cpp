#include "Camera.h"

#include <cmath>
#include <algorithm>

namespace {
constexpr float pi = 3.14159265358979323846f;
}

void Camera::setTarget(Vec3 target)
{
    target_ = target;
}

void Camera::orbit(float yawDeltaDegrees, float pitchDeltaDegrees)
{
    // Keep yaw bounded and pitch away from the poles, where the up vector fails.
    yawDegrees_ = std::remainder(yawDegrees_ + yawDeltaDegrees, 360.0f);
    pitchDegrees_ = std::clamp(pitchDegrees_ + pitchDeltaDegrees, minPitch_, maxPitch_);
}

void Camera::zoom(float distanceDelta)
{
    // Zoom changes the eye-to-target distance, not the perspective field of view.
    distance_ = std::clamp(distance_ + distanceDelta, minDistance_, maxDistance_);
}

Matrix4 Camera::viewMatrix() const
{
    const float yaw = yawDegrees_ * pi / 180.0f;
    const float pitch = pitchDegrees_ * pi / 180.0f;
    // Pitch controls height; the remaining horizontal distance rotates with yaw.
    const float horizontalDistance = distance_ * std::cos(pitch);
    const Vec3 eye = {
        target_.x + horizontalDistance * std::sin(yaw),
        target_.y + distance_ * std::sin(pitch),
        target_.z + horizontalDistance * std::cos(yaw)
    };
    return Matrix4::lookAt(eye, target_, {0, 1, 0});
}

Matrix4 Camera::projectionMatrix(int width, int height) const
{
    // Keep a valid aspect ratio if a window reports zero dimensions while minimized.
    const int safeWidth = std::max(width, 1);
    const int safeHeight = std::max(height, 1);
    return Matrix4::perspective(60.0f, static_cast<float>(safeWidth) / safeHeight,
                                0.1f, 100.0f);
}
