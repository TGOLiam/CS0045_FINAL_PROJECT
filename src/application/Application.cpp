#include "Application.h"

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <algorithm>
#include <chrono>
#include <iostream>

int Application::run(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitContextVersion(2, 1);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    windowId_ = glutCreateWindow("City Helicopter Simulator");
    if (windowId_ <= 0) {
        std::cerr << "Could not create the OpenGL window.\n";
        return 1;
    }

    const GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        std::cerr << "Could not initialize GLEW: "
                  << glewGetErrorString(glewStatus) << '\n';
        glutDestroyWindow(windowId_);
        windowId_ = 0;
        return 1;
    }

    // Initialize shared meshes after creating the OpenGL context.
    if (!renderer_.initialize()) {
        std::cerr << "Could not initialize renderer.\n";
        glutDestroyWindow(windowId_);
        windowId_ = 0;
        return 1;
    }

    if (!city_.initialize(renderer_)) {
        std::cerr << "Could not initialize city geometry.\n";
        renderer_.shutdown();
        glutDestroyWindow(windowId_);
        windowId_ = 0;
        return 1;
    }

    // Start above the highest surface directly beneath the initial position.
    const Vec3 start = helicopter_.position();
    float spawnSurface = city_.groundHeight();
    for (const Aabb& box : city_.collisionBounds()) {
        if (start.x >= box.min.x && start.x <= box.max.x &&
            start.z >= box.min.z && start.z <= box.max.z) {
            spawnSurface = std::max(spawnSurface, box.max.y);
        }
    }
    helicopter_.placeAbove(spawnSurface, customization_.scale());

    glutSetWindowData(this);
    // FreeGLUT requires a display callback; the application loop draws frames.
    glutDisplayFunc([]() {});
    glutReshapeFunc([](int width, int height) {
        auto* app = static_cast<Application*>(glutGetWindowData());
        app->width_ = std::max(width, 1);
        app->height_ = std::max(height, 1);
    });
    input_.initialize();
    glutCloseFunc([]() {
        auto* app = static_cast<Application*>(glutGetWindowData());
        if (!app) return;
        // Release renderer meshes before FreeGLUT destroys the context.
        app->renderer_.shutdown();
        app->windowId_ = 0;
        app->quitRequested_ = true;
    });

    glutFullScreen();
    auto previous = std::chrono::steady_clock::now();
    while (!quitRequested_) {
        glutMainLoopEvent();
        if (quitRequested_) break;

        const auto now = std::chrono::steady_clock::now();
        const float deltaTime = std::min(
            std::chrono::duration<float>(now - previous).count(), 0.1f);
        previous = now;
        update(deltaTime);
        if (quitRequested_) break;
        render();
        glutSwapBuffers();
    }
    input_.shutdown();
    if (windowId_ != 0) {
        // Release renderer meshes before destroying the window.
        renderer_.shutdown();
        // Explicit destruction can invoke the close callback after window data
        // is unavailable; cleanup has already happened on this path.
        glutCloseFunc(nullptr);
        glutDestroyWindow(windowId_);
        windowId_ = 0;
    }
    return 0;
}

void Application::update(float deltaTime)
{
    // Advance city traffic and pedestrians before anything renders.
    city_.update(deltaTime);
    // Route forward and sideways input in the helicopter's local directions.
    const float forward = static_cast<float>(input_.isKeyPressed('w')) - static_cast<float>(input_.isKeyPressed('s'));
    const float sideways = static_cast<float>(input_.isKeyPressed('d')) - static_cast<float>(input_.isKeyPressed('a'));
    const float vertical = static_cast<float>(input_.isKeyPressed(' ')) - static_cast<float>(input_.isControlPressed());
    const float rudder = static_cast<float>(input_.isKeyPressed('e')) - static_cast<float>(input_.isKeyPressed('q'));
    const FlightInput flightInput = {forward, vertical, sideways, rudder};
    // Keep drag unchanged while scaling acceleration and velocity limits, so
    // each size reaches its speed on a comparable timescale.
    const float speed = customization_.flightSpeedMultiplier();
    const FlightTuning tuning = {12.0f * speed, 9.0f * speed, 0.8f, 1.2f,
                                 12.0f * speed, 6.0f * speed};
    // Fixed physics steps keep acceleration and rotor motion consistent across frame rates.
    constexpr double physicsStep = 1.0 / 120.0;
    physicsAccumulator_ += deltaTime;
    while (physicsAccumulator_ >= physicsStep) {
        helicopter_.update(static_cast<float>(physicsStep), flightInput, tuning,
                           customization_.scale(), city_.collisionBounds(), city_.groundHeight());
        physicsAccumulator_ -= physicsStep;
    }
    camera_.setTarget(helicopter_.position());
    if (input_.isKeyPressed(27)) quitRequested_ = true;
    int deltaX = 0;
    int deltaY = 0;
    input_.consumeMouseDelta(deltaX, deltaY);
    camera_.orbit(-deltaX * 0.25f, deltaY * 0.25f);
    camera_.zoom(-static_cast<float>(input_.consumeWheelDelta()));
    camera_.updateObstruction(deltaTime, city_.collisionBounds(), city_.groundHeight());
}

void Application::render()
{
    const Vec3 helicopterPosition = helicopter_.position();
    renderer_.beginFrame(width_, height_, {0.08f, 0.1f, 0.14f, 1.0f});
    renderer_.begin3D(camera_.viewMatrix(), camera_.projectionMatrix(width_, height_));
    renderer_.drawSkybox(camera_.position());
    city_.render(renderer_);
    const float helicopterScale = customization_.scale();
    const Matrix4 helicopterTransform = Matrix4::translation(helicopterPosition.x,
                                                               helicopterPosition.y,
                                                               helicopterPosition.z)
                                      * Matrix4::scale(helicopterScale,
                                                       helicopterScale,
                                                       helicopterScale);
    helicopter_.draw(renderer_, helicopterTransform);
}
