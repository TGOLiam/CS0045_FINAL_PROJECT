#pragma once

#include "../renderer/Renderer.h"

class Helicopter
{
public:
    void draw(const Renderer& renderer, const Matrix4& worldTransform) const;
};
