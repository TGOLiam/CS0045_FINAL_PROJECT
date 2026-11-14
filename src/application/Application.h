#pragma once

#include "AppState.h"
#include "../renderer/Renderer.h"
#include "../camera/Camera.h"
#include "../input/Input.h"
#include "../helicopter/Helicopter.h"
#include "../city/City.h"
#include "../customization/CustomizationSettings.h"
#include "../audio/Audio.h"

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
    double physicsAccumulator_ = 0.0;
    AppState appState_ = AppState::MainMenu;
    Input input_;
    Renderer renderer_;
    Camera camera_;
    Helicopter helicopter_;
    CustomizationSettings customization_;
    City city_;
    Audio audio_;
};
