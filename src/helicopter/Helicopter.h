#pragma once

#include "../renderer/Renderer.h"

class Helicopter
{
public:
    void update(float deltaTime);
    void draw(const Renderer& renderer, const Matrix4& worldTransform) const;

private:
    float mainRotorDegrees_ = 0.0f;
    float tailRotorDegrees_ = 0.0f;

};
