#pragma once

#include "../renderer/Renderer.h"
#include "../physics/Physics.h"

class Helicopter
{
public:
    void update(float deltaTime, FlightInput input, const FlightTuning& tuning);
    void draw(const Renderer& renderer, const Matrix4& worldTransform) const;
    Vec3 position() const { return motion_.position; }
    // Conservative box includes the spinning rotors and the 16-degree visual tilt.
    Aabb collisionBounds(float scale) const;

private:
    float mainRotorDegrees_ = 0.0f;
    float tailRotorDegrees_ = 0.0f;
    // The model's nose points along +X; altitude control will use motion_.
    // Start above the 32.5-unit central tower and its rooftop helipad.
    MotionState motion_ = {{0.0f, 34.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    float forwardTilt_ = 0.0f;
    float sidewaysTilt_ = 0.0f;
};
