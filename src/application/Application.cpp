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

    glutSetWindowData(this);
    // FreeGLUT requires a display callback; the application loop draws frames.
    glutDisplayFunc([]() {});
    glutReshapeFunc([](int width, int height) {
        auto* app = static_cast<Application*>(glutGetWindowData());
        app->width_ = std::max(width, 1);
        app->height_ = std::max(height, 1);
    });
    // Temporary exit binding until Input is integrated.
    glutKeyboardFunc([](unsigned char key, int, int) {
        if (key == 27) {
            static_cast<Application*>(glutGetWindowData())->quitRequested_ = true;
        }
    });
    glutCloseFunc([]() {
        auto* app = static_cast<Application*>(glutGetWindowData());
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
        // Input events will be routed here when the Input subsystem is ready.
        update(deltaTime);
        render();
        glutSwapBuffers();
    }
    if (windowId_ != 0) {
        // Release renderer meshes before destroying the window.
        renderer_.shutdown();
        glutDestroyWindow(windowId_);
        windowId_ = 0;
    }
    return 0;
}

void Application::update(float deltaTime)
{
    (void)deltaTime;
}

void Application::render()
{
    renderer_.beginFrame(width_, height_, {0.08f, 0.1f, 0.14f, 1.0f});
    renderer_.begin3D(camera_.viewMatrix(), camera_.projectionMatrix(width_, height_));
    // Temporary Stage 1 scene: replace this cube and its face colors with scene draws.
    const Color colors[] = {
        {0.9f, 0.2f, 0.2f}, {0.2f, 0.7f, 0.9f},
        {0.2f, 0.8f, 0.3f}, {0.9f, 0.7f, 0.2f},
        {0.7f, 0.3f, 0.9f}, {0.9f, 0.5f, 0.2f}
    };
    // Preserve the test cube's previous size while using a reusable unit cube.
    const Matrix4 model = Matrix4::scale(2.0f, 2.0f, 2.0f);
    for (int face = 0; face < 6; ++face) {
        renderer_.drawRange(MeshId::Cube, model, colors[face], face * 6, 6);
    }
}
