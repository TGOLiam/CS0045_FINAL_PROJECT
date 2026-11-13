#pragma once

#include "../renderer/Renderer.h"

class Helicopter
{
public:
    void update(float deltaTime, float forward, float sideways);
    void draw(const Renderer& renderer, const Matrix4& worldTransform) const;
    Vec3 position() const { return position_; }

private:
    float mainRotorDegrees_ = 0.0f;
    float tailRotorDegrees_ = 0.0f;
    // The model's nose points along +X; position_.y is the fixed altitude.
    // Start above the 32.5-unit central tower and its rooftop helipad.
    Vec3 position_ = {0.0f, 33.65f, 0.0f};
    float forwardTilt_ = 0.0f;
    float sidewaysTilt_ = 0.0f;
};
