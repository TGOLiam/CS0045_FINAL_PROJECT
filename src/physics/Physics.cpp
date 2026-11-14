#include "Physics.h"
#include <algorithm>
#include <cmath>

bool Physics::overlaps(const Aabb& a, const Aabb& b)
{
    return a.min.x <= b.max.x && a.max.x >= b.min.x &&
           a.min.y <= b.max.y && a.max.y >= b.min.y &&
           a.min.z <= b.max.z && a.max.z >= b.min.z;
}

void Physics::stepFlight(MotionState& state, FlightInput input, const FlightTuning& tuning, float deltaTime)
{
    if (deltaTime <= 0.0f) return;
    const float inputLength = std::sqrt(input.x * input.x + input.z * input.z);
    if (inputLength > 1.0f) {
        input.x /= inputLength;
        input.z /= inputLength;
    }
    // The model faces +X at yaw zero. Positive yaw turns it toward +Z.
    constexpr float pi = 3.14159265358979323846f;
    const float yaw = state.yawDegrees * pi / 180.0f;
    const float cosine = std::cos(yaw);
    const float sine = std::sin(yaw);
    const float worldX = input.x * cosine - input.z * sine;
    const float worldZ = input.x * sine + input.z * cosine;
    input.y = std::clamp(input.y, -1.0f, 1.0f);
    const float horizontalDamping = std::exp(-tuning.horizontalDrag * deltaTime);
    const float verticalDamping = std::exp(-tuning.verticalDrag * deltaTime);
    state.velocity.x = (state.velocity.x + worldX * tuning.horizontalAcceleration * deltaTime) * horizontalDamping;
    state.velocity.z = (state.velocity.z + worldZ * tuning.horizontalAcceleration * deltaTime) * horizontalDamping;
    state.velocity.y = (state.velocity.y + input.y * tuning.verticalAcceleration * deltaTime) * verticalDamping;
    const float horizontalSpeed = std::sqrt(state.velocity.x * state.velocity.x + state.velocity.z * state.velocity.z);
    if (horizontalSpeed > tuning.maxHorizontalSpeed) {
        const float ratio = tuning.maxHorizontalSpeed / horizontalSpeed;
        state.velocity.x *= ratio;
        state.velocity.z *= ratio;
    }
    state.velocity.y = std::clamp(state.velocity.y, -tuning.maxVerticalSpeed, tuning.maxVerticalSpeed);
    // Rudder applies angular acceleration. Angular drag gradually stops the
    // turn after release; opposite rudder brakes before reversing it.
    const float rudder = std::clamp(input.rudder, -1.0f, 1.0f);
    const float angularDamping = std::exp(-tuning.rudderDrag * deltaTime);
    state.yawVelocity = (state.yawVelocity + rudder * tuning.rudderAcceleration * deltaTime) * angularDamping;
    state.yawVelocity = std::clamp(state.yawVelocity, -tuning.maxRudderSpeed, tuning.maxRudderSpeed);
    state.yawDegrees = std::remainder(state.yawDegrees + state.yawVelocity * deltaTime, 360.0f);
    state.position.x += state.velocity.x * deltaTime;
    state.position.y += state.velocity.y * deltaTime;
    state.position.z += state.velocity.z * deltaTime;
}

Aabb Physics::placeBounds(const Aabb& local, Vec3 origin, float scale)
{
    return {
        {origin.x + local.min.x * scale, origin.y + local.min.y * scale, origin.z + local.min.z * scale},
        {origin.x + local.max.x * scale, origin.y + local.max.y * scale, origin.z + local.max.z * scale}
    };
}
