#pragma once

#include "../renderer/Renderer.h"
#include "../input/Input.h"
#include "../helicopter/Helicopter.h"

class HUD {
public:
    static void render(
        Renderer& renderer,
        const Helicopter& helicopter,
        int width,
        int height,
        float groundHeight
    );

    // Returns true if the Main Menu button was clicked.
    static bool handleInput(
        Input& input,
        int width,
        int height
    );
};