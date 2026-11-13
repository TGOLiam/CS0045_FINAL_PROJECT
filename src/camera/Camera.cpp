#include "Camera.h"

#include <cmath>

namespace {
constexpr float pi = 3.14159265358979323846f;
}

void Camera::setTarget(Vec3 target)
{
    target_ = target;
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
    // Temporary cube projection; the full camera will own its projection settings.
    return Matrix4::perspective(60.0f, static_cast<float>(width) / height, 0.1f, 100.0f);
}
