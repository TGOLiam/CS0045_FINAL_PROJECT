#include "Physics.h"
#include <algorithm>
#include <cmath>
#include <limits>

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

namespace {
SweepResult sweepBox(const Aabb& moving, Vec3 displacement, const Aabb& obstacle)
{
    const float low[] = {moving.min.x, moving.min.y, moving.min.z};
    const float high[] = {moving.max.x, moving.max.y, moving.max.z};
    const float otherLow[] = {obstacle.min.x, obstacle.min.y, obstacle.min.z};
    const float otherHigh[] = {obstacle.max.x, obstacle.max.y, obstacle.max.z};
    const float motion[] = {displacement.x, displacement.y, displacement.z};
    float entry = -std::numeric_limits<float>::infinity();
    float exit = std::numeric_limits<float>::infinity();
    int hitAxis = -1;
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(motion[axis]) < 1e-8f) {
            // A box resting against a surface can still slide along it.
            if (high[axis] <= otherLow[axis] || low[axis] >= otherHigh[axis]) return {};
            continue;
        }
        float first = (otherLow[axis] - high[axis]) / motion[axis];
        float last = (otherHigh[axis] - low[axis]) / motion[axis];
        if (first > last) std::swap(first, last);
        if (first > entry) { entry = first; hitAxis = axis; }
        exit = std::min(exit, last);
    }
    if (hitAxis < 0 || entry > exit || entry < 0.0f || entry > 1.0f || exit <= 0.0f) return {};
    SweepResult result;
    result.hit = true;
    result.fraction = entry;
    const float normal = motion[hitAxis] > 0 ? -1.0f : 1.0f;
    if (hitAxis == 0) result.normal.x = normal;
    if (hitAxis == 1) result.normal.y = normal;
    if (hitAxis == 2) result.normal.z = normal;
    return result;
}
}

void Physics::moveAndSlide(MotionState& state, const Aabb& localBounds, float scale,
                           const std::vector<Aabb>& obstacles, float groundHeight,
                           Vec3 displacement)
{
    constexpr float clearance = 0.001f;
    for (int iteration = 0; iteration < 4; ++iteration) {
        const Aabb moving = placeBounds(localBounds, state.position, scale);
        SweepResult nearest;
        for (const Aabb& obstacle : obstacles) {
            const SweepResult candidate = sweepBox(moving, displacement, obstacle);
            if (candidate.hit && (!nearest.hit || candidate.fraction < nearest.fraction)) nearest = candidate;
        }
        if (displacement.y < 0.0f && moving.min.y >= groundHeight) {
            const float fraction = (groundHeight - moving.min.y) / displacement.y;
            if (fraction >= 0.0f && fraction <= 1.0f && (!nearest.hit || fraction < nearest.fraction)) {
                nearest = {true, fraction, {0.0f, 1.0f, 0.0f}};
            }
        }
        const float fraction = nearest.hit ? nearest.fraction : 1.0f;
        state.position.x += displacement.x * fraction;
        state.position.y += displacement.y * fraction;
        state.position.z += displacement.z * fraction;
        if (!nearest.hit) break;
        state.position.x += nearest.normal.x * clearance;
        state.position.y += nearest.normal.y * clearance;
        state.position.z += nearest.normal.z * clearance;
        const float remaining = 1.0f - fraction;
        displacement.x *= remaining;
        displacement.y *= remaining;
        displacement.z *= remaining;
        if (nearest.normal.x != 0.0f) { displacement.x = 0.0f; state.velocity.x = 0.0f; }
        if (nearest.normal.y != 0.0f) { displacement.y = 0.0f; state.velocity.y = 0.0f; }
        if (nearest.normal.z != 0.0f) { displacement.z = 0.0f; state.velocity.z = 0.0f; }
    }
}
