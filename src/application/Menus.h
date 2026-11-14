#pragma once

#include "AppState.h"
#include "../renderer/Renderer.h"
#include "../input/Input.h"
#include "../helicopter/Helicopter.h"
#include "../customization/CustomizationSettings.h"
#include "../audio/Audio.h"

class Menus
{
public:
    static void render(
        Renderer& renderer,
        Helicopter& helicopter,
        AppState state,
        int width,
        int height,
        const CustomizationSettings& customization,
        Audio& audio
    );

    static void handleInput(
        Input& input,
        AppState& state,
        int width,
        int height,
        bool& quitRequested,
        CustomizationSettings& customization,
        Audio& audio
    );
};