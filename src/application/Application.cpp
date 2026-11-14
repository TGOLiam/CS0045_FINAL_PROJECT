#include "Application.h"
#include "Menus.h"
#include "hud.h"

#include <GL/freeglut.h>
#include <GL/glew.h>

#include <algorithm>
#include <chrono>
#include <iostream>

int Application::run(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitContextVersion(2, 1);

  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

  windowId_ = glutCreateWindow("City Helicopter Simulator");

  if (windowId_ <= 0) {
    std::cerr << "Could not create the OpenGL window.\n";
    return 1;
  }

  // Initialize GLEW.
  const GLenum glewStatus = glewInit();

  if (glewStatus != GLEW_OK) {
    std::cerr << "Could not initialize GLEW: " << glewGetErrorString(glewStatus)
              << '\n';

    glutDestroyWindow(windowId_);
    windowId_ = 0;
    return 1;
  }

  // Initialize renderer.
  if (!renderer_.initialize()) {
    std::cerr << "Could not initialize renderer.\n";

    glutDestroyWindow(windowId_);
    windowId_ = 0;
    return 1;
  }

  // Initialize city.
  if (!city_.initialize(renderer_)) {
    std::cerr << "Could not initialize city geometry.\n";

    renderer_.shutdown();
    glutDestroyWindow(windowId_);
    windowId_ = 0;
    return 1;
  }

  // Initialize audio.
  if (!audio_.initialize()) {
    std::cerr << "Warning: Audio initialization failed.\n";
    std::cerr << "The simulator will continue without audio.\n";
  }

  // Calculate helicopter spawn height.
  const Vec3 start = helicopter_.position();

  float spawnSurface = city_.groundHeight();

  for (const Aabb &box : city_.collisionBounds()) {
    if (start.x >= box.min.x && start.x <= box.max.x && start.z >= box.min.z &&
        start.z <= box.max.z) {
      spawnSurface = std::max(spawnSurface, box.max.y);
    }
  }

  helicopter_.placeAbove(spawnSurface, customization_.scale());

  // Connect application to FreeGLUT.
  glutSetWindowData(this);

  glutDisplayFunc([]() {});

  glutReshapeFunc([](int width, int height) {
    auto *app = static_cast<Application *>(glutGetWindowData());

    if (!app)
      return;

    app->width_ = std::max(width, 1);
    app->height_ = std::max(height, 1);
  });

  input_.initialize();

  // Handle window closing.
  glutCloseFunc([]() {
    auto *app = static_cast<Application *>(glutGetWindowData());

    if (!app)
      return;

    app->audio_.stopHelicopterSound();
    app->audio_.shutdown();

    app->renderer_.shutdown();

    app->windowId_ = 0;
    app->quitRequested_ = true;
  });

  glutFullScreen();

  auto previous = std::chrono::steady_clock::now();

  // Main application loop.
  while (!quitRequested_) {
    glutMainLoopEvent();

    if (quitRequested_)
      break;

    const auto now = std::chrono::steady_clock::now();

    const float deltaTime =
        std::min(std::chrono::duration<float>(now - previous).count(), 0.1f);

    previous = now;

    update(deltaTime);

    if (quitRequested_)
      break;

    render();

    glutSwapBuffers();
  }

  // Final cleanup.
  audio_.stopHelicopterSound();
  audio_.shutdown();

  input_.shutdown();

  if (windowId_ != 0) {
    renderer_.shutdown();

    glutCloseFunc(nullptr);
    glutDestroyWindow(windowId_);

    windowId_ = 0;
  }

  return 0;
}

// --------------------------------------------------
// UPDATE
// --------------------------------------------------

void Application::update(float deltaTime) {
  // MENU STATES
  if (appState_ != AppState::Playing) {
    HUD::reset();

    // Process menu navigation and customization.
    Menus::handleInput(input_, appState_, width_, height_, quitRequested_,
                       customization_, audio_);

    // Manage audio depending on the current state.
    if (appState_ == AppState::Playing) {
      audio_.startHelicopterSound();
    } else {
      audio_.stopHelicopterSound();
    }

    return;
  }

  // GAMEPLAY STATE

  // Ensure helicopter sound is playing.
  audio_.startHelicopterSound();

  // Update city traffic and pedestrians.
  city_.update(deltaTime);

  // Handle HUD button clicks.
  if (HUD::handleInput(input_, width_, height_)) {
    audio_.playUIClick();
    audio_.stopHelicopterSound();
    appState_ = AppState::MainMenu;
    physicsAccumulator_ = 0.0;
    return;
  }

  // Read helicopter movement input.
  const float forward = static_cast<float>(input_.isKeyPressed('w')) -
                        static_cast<float>(input_.isKeyPressed('s'));

  const float sideways = static_cast<float>(input_.isKeyPressed('d')) -
                         static_cast<float>(input_.isKeyPressed('a'));

  const float vertical = static_cast<float>(input_.isKeyPressed(' ')) -
                         static_cast<float>(input_.isControlPressed());

  const float rudder = static_cast<float>(input_.isKeyPressed('e')) -
                       static_cast<float>(input_.isKeyPressed('q'));

  const FlightInput flightInput = {forward, vertical, sideways, rudder};

  // Size changes acceleration only. Every size shares the faster top speed.
  const float acceleration = customization_.flightAccelerationMultiplier();
  constexpr float maxHorizontalSpeed = 42.0f;
  constexpr float maxVerticalSpeed = 21.0f;
  constexpr float baseRudderAcceleration = 120.0f;
  constexpr float baseMaxRudderSpeed = 45.0f;
  const FlightTuning tuning = {12.0f * acceleration, 9.0f * acceleration,
                               0.8f, 1.2f, maxHorizontalSpeed, maxVerticalSpeed,
                               baseRudderAcceleration * acceleration, 2.0f,
                               baseMaxRudderSpeed * acceleration};

  // Fixed physics updates.
  constexpr double physicsStep = 1.0 / 120.0;

  physicsAccumulator_ += deltaTime;

  while (physicsAccumulator_ >= physicsStep) {
    helicopter_.update(static_cast<float>(physicsStep), flightInput, tuning,
                       customization_.scale(), city_.collisionBounds(),
                       city_.groundHeight());

    physicsAccumulator_ -= physicsStep;
  }

  HUD::update(helicopter_, city_.groundHeight(), deltaTime);

  // Update camera target.
  camera_.setTarget(helicopter_.position());

  // ESC exits gameplay.
  if (input_.isKeyPressed(27)) {
    audio_.stopHelicopterSound();
    quitRequested_ = true;
    return;
  }

  // Mouse camera controls.
  int deltaX = 0;
  int deltaY = 0;

  input_.consumeMouseDelta(deltaX, deltaY);

  camera_.orbit(-deltaX * 0.25f, deltaY * 0.25f);

  camera_.zoom(-static_cast<float>(input_.consumeWheelDelta()));

  camera_.updateObstruction(deltaTime, city_.collisionBounds(),
                            city_.groundHeight());
}

// --------------------------------------------------
// RENDER
// --------------------------------------------------

void Application::render() {
  const Vec3 helicopterPosition = helicopter_.position();

  renderer_.beginFrame(width_, height_, {0.035f, 0.055f, 0.085f, 1.0f});

  if (appState_ == AppState::Playing) {
    // Gameplay rendering.
    renderer_.begin3D(camera_.viewMatrix(),
                      camera_.projectionMatrix(width_, height_));

    renderer_.drawSkybox(camera_.position());

    city_.render(renderer_);

    // Apply selected helicopter size.
    const float helicopterScale = customization_.scale();

    const Matrix4 helicopterTransform =
        Matrix4::translation(helicopterPosition.x, helicopterPosition.y,
                             helicopterPosition.z) *
        Matrix4::scale(helicopterScale, helicopterScale, helicopterScale);

    // Render customized helicopter.
    helicopter_.draw(renderer_, helicopterTransform,
                     customization_.bodyColor());

    // Render in-game HUD.
    HUD::render(renderer_, helicopter_, width_, height_, city_.groundHeight());
  } else {
    // Menu rendering.
    Menus::render(renderer_, helicopter_, appState_, width_, height_,
                  customization_, audio_);
  }
}
