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

void Camera::updateObstruction(float deltaTime, const std::vector<Aabb>& obstacles, float groundHeight)
{
    constexpr float radius = 0.4f;
    constexpr float clearance = 0.05f;
    constexpr float piDegrees = pi / 180.0f;
    const float yaw = yawDegrees_ * piDegrees;
    const float pitch = pitchDegrees_ * piDegrees;
    const Vec3 direction = {std::cos(pitch) * std::sin(yaw),
                            std::sin(pitch),
                            std::cos(pitch) * std::cos(yaw)};
    const Vec3 displacement = {direction.x * distance_, direction.y * distance_, direction.z * distance_};
    const Aabb cameraBox = {{target_.x-radius, target_.y-radius, target_.z-radius},
                            {target_.x+radius, target_.y+radius, target_.z+radius}};
    float allowedDistance = distance_;
    for (const Aabb& box : obstacles) {
        const SweepResult hit = Physics::sweepAabb(cameraBox, displacement, box);
        if (hit.hit) allowedDistance = std::min(allowedDistance, std::max(0.0f, hit.fraction * distance_ - clearance));
    }
    if (displacement.y < 0.0f && cameraBox.min.y >= groundHeight) {
        const float fraction = (groundHeight - cameraBox.min.y) / displacement.y;
        if (fraction >= 0.0f && fraction <= 1.0f)
            allowedDistance = std::min(allowedDistance, std::max(0.0f, fraction * distance_ - clearance));
    }
    // Move inward immediately; ease outward to avoid a sudden camera jump.
    if (allowedDistance < actualDistance_) actualDistance_ = allowedDistance;
    else actualDistance_ += (allowedDistance - actualDistance_) * (1.0f - std::exp(-5.0f * std::max(deltaTime, 0.0f)));
    // lookAt requires eye and target to differ even in a very tight space.
    actualDistance_ = std::max(actualDistance_, 0.001f);
}

Vec3 Camera::position() const
{
    const float yaw = yawDegrees_ * pi / 180.0f;
    const float pitch = pitchDegrees_ * pi / 180.0f;
    // Pitch controls height; the remaining horizontal distance rotates with yaw.
    const float horizontalDistance = actualDistance_ * std::cos(pitch);
    return {
        target_.x + horizontalDistance * std::sin(yaw),
        target_.y + actualDistance_ * std::sin(pitch),
        target_.z + horizontalDistance * std::cos(yaw)
    };
}

Matrix4 Camera::viewMatrix() const
{
    return Matrix4::lookAt(position(), target_, {0, 1, 0});
}

Matrix4 Camera::projectionMatrix(int width, int height) const
{
    // Keep a valid aspect ratio if a window reports zero dimensions while minimized.
    const int safeWidth = std::max(width, 1);
    const int safeHeight = std::max(height, 1);
    return Matrix4::perspective(60.0f, static_cast<float>(safeWidth) / safeHeight,
                                0.1f, 1000.0f);
}
