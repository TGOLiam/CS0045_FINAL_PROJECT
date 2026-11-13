#include "Camera.h"

Matrix4 Camera::viewMatrix() const
{
    // Temporary cube viewpoint; replace with the orbit camera's tracked target.
    return Matrix4::lookAt({4, 3, 5}, {0, 0, 0}, {0, 1, 0});
}

Matrix4 Camera::projectionMatrix(int width, int height) const
{
    // Temporary cube projection; the full camera will own its projection settings.
    return Matrix4::perspective(60.0f, static_cast<float>(width) / height, 0.1f, 100.0f);
}
