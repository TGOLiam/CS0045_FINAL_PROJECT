#include "Helicopter.h"
#include <cmath>

Aabb Helicopter::collisionBounds(float scale) const
{
    // Bounds surround the model and rotor sweep, including combined tilt.
    // The box remains aligned to world axes as the model leans.
    const Aabb local = {{-3.1f, -1.8f, -3.1f}, {3.1f, 1.8f, 3.1f}};
    return Physics::placeBounds(local, motion_.position, scale);
}

void Helicopter::update(float deltaTime, FlightInput input, const FlightTuning& tuning)
{
    // Fixed rotor speeds: RPM converted to degrees per second.
    constexpr float mainRotorRpm = 180.0f;
    constexpr float tailRotorRpm = 360.0f;
    mainRotorDegrees_ = std::fmod(mainRotorDegrees_ + mainRotorRpm * 6.0f * deltaTime, 360.0f);
    tailRotorDegrees_ = std::fmod(tailRotorDegrees_ + tailRotorRpm * 6.0f * deltaTime, 360.0f);
    // Normalize diagonal movement so it is no faster than a single direction.
    float forward = input.x;
    float sideways = input.z;
    const float length = std::sqrt(forward * forward + sideways * sideways);
    if (length > 1.0f) {
        forward /= length;
        sideways /= length;
    }
    // Acceleration and drag allow momentum to persist after input is released.
    Physics::stepFlight(motion_, input, tuning, deltaTime);
    // Gradually lean in the movement direction and return to level at rest.
    const float blend = 1.0f - std::exp(-6.0f * deltaTime);
    forwardTilt_ += (-forward * 16.0f - forwardTilt_) * blend;
    sidewaysTilt_ += (sideways * 16.0f - sidewaysTilt_) * blend;
}

void Helicopter::draw(const Renderer& renderer, const Matrix4& worldTransform) const
{
    // Tilt the whole model around its local origin before positioning its parts.
    const Matrix4 tiltedTransform = worldTransform
        * Matrix4::rotation(-motion_.yawDegrees, {0.0f, 1.0f, 0.0f})
        * Matrix4::rotation(forwardTilt_, {0.0f, 0.0f, 1.0f})
        * Matrix4::rotation(sidewaysTilt_, {1.0f, 0.0f, 0.0f});
    // Helicopter body
    renderer.draw(
        MeshId::Cube,
        tiltedTransform * Matrix4::translation(0.0f, 0.0f, 0.0f)
                       * Matrix4::scale(1.4f, 0.5f, 0.65f),
        {0.8f, 0.15f, 0.12f}
    );

    // Cockpit
    renderer.draw(
        MeshId::Cube,
        tiltedTransform * Matrix4::translation(0.48f, 0.12f, 0.0f)
                       * Matrix4::scale(0.55f, 0.4f, 0.58f),
        {0.2f, 0.65f, 0.85f}
    );

    // Tail boom
    renderer.draw(
        MeshId::Cube,
        tiltedTransform * Matrix4::translation(-1.15f, 0.12f, 0.0f)
                       * Matrix4::scale(1.5f, 0.16f, 0.18f),
        {0.65f, 0.12f, 0.1f}
    );

    // Tail fin
    renderer.draw(
        MeshId::Cube,
        tiltedTransform * Matrix4::translation(-1.8f, 0.35f, 0.0f)
                       * Matrix4::scale(0.3f, 0.55f, 0.12f),
        {0.8f, 0.15f, 0.12f}
    );

    // Main rotor mast
    renderer.draw(
        MeshId::Cube,
        tiltedTransform * Matrix4::translation(0.0f, 0.42f, 0.0f)
                       * Matrix4::scale(0.12f, 0.4f, 0.12f),
        {0.3f, 0.3f, 0.3f}
    );

    // Main rotor blades
    renderer.draw(
        MeshId::Cube,
        tiltedTransform * Matrix4::translation(0.0f, 0.65f, 0.0f)
                       * Matrix4::rotation(mainRotorDegrees_, {0.0f, 1.0f, 0.0f})
                       * Matrix4::scale(2.8f, 0.06f, 0.16f),
        {0.12f, 0.12f, 0.12f}
    );

    renderer.draw(
        MeshId::Cube,
        tiltedTransform * Matrix4::translation(0.0f, 0.65f, 0.0f)
                       * Matrix4::rotation(mainRotorDegrees_ + 90.0f, {0.0f, 1.0f, 0.0f})
                       * Matrix4::scale(2.8f, 0.06f, 0.16f),
        {0.12f, 0.12f, 0.12f}
    );

    // Tail rotor sits beside the fin; both blades rotate in the XY plane.
    renderer.draw(
        MeshId::Cube,
        tiltedTransform * Matrix4::translation(-1.95f, 0.25f, 0.12f)
                       * Matrix4::rotation(tailRotorDegrees_, {0.0f, 0.0f, 1.0f})
                       * Matrix4::scale(0.08f, 0.55f, 0.1f),
        {0.15f, 0.15f, 0.15f}
    );

    renderer.draw(
        MeshId::Cube,
        tiltedTransform * Matrix4::translation(-1.95f, 0.25f, 0.12f)
                       * Matrix4::rotation(tailRotorDegrees_ + 90.0f, {0.0f, 0.0f, 1.0f})
                       * Matrix4::scale(0.08f, 0.55f, 0.1f),
        {0.15f, 0.15f, 0.15f}
    );

    // Landing skids
    for (int side = -1; side <= 1; side += 2)
    {
        // Vertical supports
        renderer.draw(
            MeshId::Cube,
            tiltedTransform * Matrix4::translation(-0.1f, -0.38f, side * 0.28f)
                           * Matrix4::scale(0.1f, 0.45f, 0.1f),
            {0.25f, 0.25f, 0.25f}
        );

        // Horizontal skid
        renderer.draw(
            MeshId::Cube,
            tiltedTransform * Matrix4::translation(-0.1f, -0.65f, side * 0.35f)
                           * Matrix4::scale(1.7f, 0.08f, 0.1f),
            {0.2f, 0.2f, 0.2f}
        );
    }
}
