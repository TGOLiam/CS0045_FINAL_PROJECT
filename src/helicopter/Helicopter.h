#pragma once

#include "../renderer/Renderer.h"
#include "../customization/CustomizationSettings.h"

class Helicopter
{
public:
    void draw(
        const Renderer& renderer,
        const Matrix4& worldTransform,
        const CustomizationSettings& settings
    ) const;
};