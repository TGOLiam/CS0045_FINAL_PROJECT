#pragma once

#include "AppState.h"
#include "../renderer/Renderer.h"
#include "../camera/Camera.h"

class Application {
public:
    int run(int argc, char** argv);

private:
    void update(float deltaTime);
    void render();

    int windowId_ = 0;
    int width_ = 1;
    int height_ = 1;
    bool quitRequested_ = false;
    Renderer renderer_;
    Camera camera_;
};
