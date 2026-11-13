#include "City.h"

#include "../renderer/Renderer.h"

void City::render(Renderer& renderer) const
{
    // The unit quad is in XY; rotate it onto XZ at ground level.
    const Matrix4 ground = Matrix4::rotation(-90.0f, {1.0f, 0.0f, 0.0f})
                         * Matrix4::scale(400.0f, 400.0f, 1.0f);
    renderer.draw(MeshId::Quad, ground, {0.85f, 0.83f, 0.77f});
}
