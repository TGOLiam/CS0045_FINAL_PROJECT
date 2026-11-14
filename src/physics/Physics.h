#pragma once

#include "../math/Matrix4.h"
#include <vector>

// Helicopter owns this state; Physics operates on it without keeping a copy.
struct MotionState {
    Vec3 position = {0.0f, 0.0f, 0.0f};
    Vec3 velocity = {0.0f, 0.0f, 0.0f};
    // Positive yaw is a right turn, measured in degrees.
    float yawDegrees = 0.0f;
    float yawVelocity = 0.0f;
};

// X is forward, Z is right relative to the helicopter; Y is vertical.
struct FlightInput {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float rudder = 0.0f; // -1 turns left, +1 turns right.
};

struct FlightTuning {
    float horizontalAcceleration;
    float verticalAcceleration;
    float horizontalDrag;
    float verticalDrag;
    float maxHorizontalSpeed;
    float maxVerticalSpeed;
    float rudderAcceleration = 180.0f;
    float rudderDrag = 2.0f;
    float maxRudderSpeed = 90.0f;
};

// World-axis-aligned bounds. min and max are positions, not half-extents.
struct Aabb {
    Vec3 min;
    Vec3 max;
};

// A sweep reports first contact along a displacement: fraction 0..1 and the
// outward surface normal. A miss uses hit=false and fraction=1.
struct SweepResult {
    bool hit = false;
    float fraction = 1.0f;
    Vec3 normal = {0.0f, 0.0f, 0.0f};
};

namespace Physics {
// Touching faces count as contact; bounds must have min <= max on each axis.
bool overlaps(const Aabb& a, const Aabb& b);

// Place local bounds at an origin using a positive, uniform model scale.
Aabb placeBounds(const Aabb& local, Vec3 origin, float scale);

// Rotate local horizontal input by heading, then integrate world-space velocity.
void stepFlight(MotionState& state, FlightInput input, const FlightTuning& tuning, float deltaTime);

// Move a world-aligned box through static obstacles, removing velocity into
// contact surfaces while preserving tangential motion.
void moveAndSlide(MotionState& state, const Aabb& localBounds, float scale,
                  const std::vector<Aabb>& obstacles, float groundHeight,
                  Vec3 displacement);
}
