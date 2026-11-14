#include "Helicopter.h"

#include <cmath>

Aabb Helicopter::localCollisionBounds() {
  constexpr float hitboxScale = 0.75f;
  return {{-3.1f * hitboxScale, -1.8f * hitboxScale, -3.1f * hitboxScale},
          {3.1f * hitboxScale, 1.8f * hitboxScale, 3.1f * hitboxScale}};
}

Aabb Helicopter::collisionBounds(float scale) const {
  return Physics::placeBounds(localCollisionBounds(), motion_.position, scale);
}

void Helicopter::placeAbove(float surfaceHeight, float scale) {
  motion_.position.y =
      surfaceHeight - localCollisionBounds().min.y * scale + 0.01f;
  motion_.velocity = {0.0f, 0.0f, 0.0f};
}

void Helicopter::update(float deltaTime, FlightInput input,
                        const FlightTuning &tuning, float scale,
                        const std::vector<Aabb> &obstacles,
                        float groundHeight) {
    constexpr float mainRotorRpm = 240.0f;
    constexpr float tailRotorRpm = 480.0f;
  mainRotorDegrees_ =
      std::fmod(mainRotorDegrees_ + mainRotorRpm * 6.0f * deltaTime, 360.0f);
  tailRotorDegrees_ =
      std::fmod(tailRotorDegrees_ + tailRotorRpm * 6.0f * deltaTime, 360.0f);

  float forward = input.x;
  float sideways = input.z;
  const float length = std::sqrt(forward * forward + sideways * sideways);
  if (length > 1.0f) {
    forward /= length;
    sideways /= length;
  }

  const Vec3 previous = motion_.position;
  Physics::stepFlight(motion_, input, tuning, deltaTime);
  const Vec3 displacement = {motion_.position.x - previous.x,
                             motion_.position.y - previous.y,
                             motion_.position.z - previous.z};
  motion_.position = previous;
  Physics::moveAndSlide(motion_, localCollisionBounds(), scale, obstacles,
                        groundHeight, displacement);

  const float blend = 1.0f - std::exp(-6.0f * deltaTime);
  forwardTilt_ += (-forward * 16.0f - forwardTilt_) * blend;
  sidewaysTilt_ += (sideways * 16.0f - sidewaysTilt_) * blend;
}

void Helicopter::draw(const Renderer &renderer, const Matrix4 &worldTransform,
                      Color bodyColor) const {
  const Matrix4 aircraft =
      worldTransform *
      Matrix4::rotation(-motion_.yawDegrees, {0.0f, 1.0f, 0.0f}) *
      Matrix4::rotation(forwardTilt_, {0.0f, 0.0f, 1.0f}) *
      Matrix4::rotation(sidewaysTilt_, {1.0f, 0.0f, 0.0f});

  const Color darkBody = {bodyColor.r * 0.62f, bodyColor.g * 0.62f,
                          bodyColor.b * 0.62f, bodyColor.a};
  const Color glass = {0.12f + bodyColor.r * 0.14f, 0.52f + bodyColor.g * 0.22f,
                       0.70f + bodyColor.b * 0.16f, bodyColor.a};
  constexpr Color metal = {0.12f, 0.14f, 0.17f};
  constexpr Color hubMetal = {0.38f, 0.42f, 0.47f};
  constexpr Color yellow = {0.95f, 0.76f, 0.18f};

  // All dimensions use the same reference origin: body centre at (0, 0, 0),
  // +X nose, +Y up. This replaces the former mixed old/new part layout.
  const auto box = [&](Vec3 position, Vec3 size, Color color) {
    renderer.draw(MeshId::Cube,
                  aircraft *
                      Matrix4::translation(position.x, position.y, position.z) *
                      Matrix4::scale(size.x, size.y, size.z),
                  color);
  };

  // Fuselage and nose.
  box({0.0f, 0.0f, 0.0f}, {2.0f, 1.0f, 1.1f}, bodyColor);
  box({0.0f, -0.48f, 0.0f}, {1.8f, 0.10f, 0.90f}, darkBody);
  box({1.25f, -0.15f, 0.0f}, {0.70f, 0.70f, 0.95f}, bodyColor);

  // Canopy and windscreen deliberately overlap the red nose and cabin.
  box({0.95f, 0.40f, 0.0f}, {0.90f, 0.75f, 1.0f}, glass);
  // Door seams remain visible for every selected paint colour.
  box({0.30f, 0.0f, 0.56f}, {0.045f, 0.58f, 0.025f}, metal);
  box({0.30f, 0.0f, -0.56f}, {0.045f, 0.58f, 0.025f}, metal);

  // Engine cowl, intake, mast, hub, and rotor form one overlapping stack.
  box({-0.35f, 0.70f, 0.0f}, {1.10f, 0.40f, 0.80f}, darkBody);
  box({-0.10f, 0.70f, 0.0f}, {0.30f, 0.18f, 0.82f}, metal);
  box({-0.35f, 1.12f, 0.0f}, {0.16f, 0.45f, 0.16f}, metal);
  box({-0.35f, 1.38f, 0.0f}, {0.35f, 0.12f, 0.35f}, hubMetal);

    // Two opposing half-blades make a single 7.5-unit rotor bar.
  for (int side = 0; side < 2; ++side) {
    const Matrix4 blade = aircraft * Matrix4::translation(-0.35f, 1.45f, 0.0f) *
                          Matrix4::rotation(mainRotorDegrees_ + side * 180.0f,
                                            {0.0f, 1.0f, 0.0f});
    renderer.draw(MeshId::Cube,
                      blade * Matrix4::translation(1.875f, 0.0f, 0.0f) *
                          Matrix4::scale(3.75f, 0.05f, 0.22f),
                  metal);
    renderer.draw(MeshId::Cube,
                      blade * Matrix4::translation(3.57f, 0.0f, 0.0f) *
                      Matrix4::scale(0.36f, 0.056f, 0.22f),
                  yellow);
  }

  // Tail boom connects directly to the fuselage, then to a fin which carries
  // both the rotor and navigation lights.
  box({-2.00f, 0.15f, 0.0f}, {2.40f, 0.30f, 0.30f}, bodyColor);
  box({-3.10f, 0.50f, 0.0f}, {0.50f, 0.90f, 0.12f}, darkBody);
  box({-2.80f, 0.15f, 0.0f}, {0.50f, 0.08f, 0.90f}, darkBody);

  const Matrix4 tailRotor =
      aircraft * Matrix4::translation(-3.15f, 0.85f, 0.12f) *
      Matrix4::rotation(tailRotorDegrees_, {0.0f, 0.0f, 1.0f});
  renderer.draw(MeshId::Cube, tailRotor * Matrix4::scale(0.07f, 1.0f, 0.06f),
                metal);
  renderer.draw(MeshId::Cube, tailRotor * Matrix4::scale(1.0f, 0.07f, 0.06f),
                metal);
  renderer.draw(MeshId::Cube,
                tailRotor * Matrix4::translation(0.0f, 0.0f, -0.05f) *
                    Matrix4::scale(0.16f, 0.16f, 0.12f),
                hubMetal);

  box({-3.10f, 0.20f, 0.06f}, {0.10f, 0.10f, 0.10f}, {0.15f, 0.90f, 0.32f});
  box({-3.10f, 0.20f, -0.06f}, {0.10f, 0.10f, 0.10f}, {0.95f, 0.18f, 0.15f});
  box({-3.25f, 0.93f, 0.0f}, {0.10f, 0.10f, 0.10f}, {0.95f, 0.95f, 0.95f});

  // Skids: two rails, each carried by front and rear supports.
  for (int side = -1; side <= 1; side += 2) {
    const float z = side * 0.60f;
    box({0.0f, -0.90f, z}, {2.60f, 0.09f, 0.13f}, metal);
    for (int support = -1; support <= 1; support += 2) {
      renderer.draw(
          MeshId::Cube,
          aircraft *
              Matrix4::translation(support * 0.57f, -0.67f, side * 0.54f) *
              Matrix4::rotation(side * 10.0f, {1.0f, 0.0f, 0.0f}) *
              Matrix4::scale(0.10f, 0.45f, 0.10f),
          metal);
    }
  }
}
