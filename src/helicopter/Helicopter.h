#pragma once

#include "../renderer/Renderer.h"
#include "../physics/Physics.h"

class Helicopter
{
public:
    void update(float deltaTime, FlightInput input,
                const FlightTuning& tuning,
                float scale,
                const std::vector<Aabb>& obstacles,
                float groundHeight);

    void placeAbove(float surfaceHeight, float scale);

    void draw(
        const Renderer& renderer,
        const Matrix4& worldTransform,
        Color bodyColor = {0.8f, 0.15f, 0.12f, 1.0f}
    ) const;

    Vec3 position() const
    {
        return motion_.position;
    }

    Vec3 velocity() const {
        return motion_.velocity;
    }

    float headingDegrees() const {
        return motion_.yawDegrees;
    }
    
    Aabb collisionBounds(float scale) const;

private:
    static Aabb localCollisionBounds();

    float mainRotorDegrees_ = 0.0f;
    float tailRotorDegrees_ = 0.0f;

    MotionState motion_ = {
        {0.0f, 34.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };

    float forwardTilt_ = 0.0f;
    float sidewaysTilt_ = 0.0f;
};
