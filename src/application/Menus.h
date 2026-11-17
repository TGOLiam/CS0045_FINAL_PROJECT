#pragma once

#include "AppState.h"
#include "../renderer/Renderer.h"
#include "../input/Input.h"
#include "../helicopter/Helicopter.h"
#include "../customization/CustomizationSettings.h"
#include "../audio/Audio.h"
#include "../city/City.h"

#include <array>

class Menus
{
public:
    bool initialize(Renderer& renderer);

    void render(
        Renderer& renderer,
        const City& city,
        AppState state,
        int width,
        int height,
        const CustomizationSettings& customization,
        const Audio& audio
    ) const;

    void update(
        Input& input,
        AppState& state,
        int width,
        int height,
        bool& quitRequested,
        CustomizationSettings& customization,
        Audio& audio,
        float deltaTime
    );

private:
    // This display model and its camera never change the flight helicopter.
    Helicopter displayHelicopter_;
    std::array<std::size_t, 45> glyphMeshes_{};
    std::array<float, 5> hover_{};
    float elapsed_ = 0.0f;
    float mouseX_ = -1.0f;
    float mouseY_ = -1.0f;
    int activeSlider_ = -1;
};
