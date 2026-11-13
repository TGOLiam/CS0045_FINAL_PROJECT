#include "Helicopter.h"

void Helicopter::draw(const Renderer& renderer, const Matrix4& worldTransform) const
{
    // Helicopter body
    renderer.draw(
        MeshId::Cube,
        worldTransform * Matrix4::translation(0.0f, 0.0f, 0.0f)
                       * Matrix4::scale(1.4f, 0.5f, 0.65f),
        {0.8f, 0.15f, 0.12f}
    );

    // Cockpit
    renderer.draw(
        MeshId::Cube,
        worldTransform * Matrix4::translation(0.48f, 0.12f, 0.0f)
                       * Matrix4::scale(0.55f, 0.4f, 0.58f),
        {0.2f, 0.65f, 0.85f}
    );

    // Tail boom
    renderer.draw(
        MeshId::Cube,
        worldTransform * Matrix4::translation(-1.15f, 0.12f, 0.0f)
                       * Matrix4::scale(1.5f, 0.16f, 0.18f),
        {0.65f, 0.12f, 0.1f}
    );

    // Tail fin
    renderer.draw(
        MeshId::Cube,
        worldTransform * Matrix4::translation(-1.8f, 0.35f, 0.0f)
                       * Matrix4::scale(0.3f, 0.55f, 0.12f),
        {0.8f, 0.15f, 0.12f}
    );

    // Main rotor mast
    renderer.draw(
        MeshId::Cube,
        worldTransform * Matrix4::translation(0.0f, 0.42f, 0.0f)
                       * Matrix4::scale(0.12f, 0.4f, 0.12f),
        {0.3f, 0.3f, 0.3f}
    );

    // Main rotor blades
    renderer.draw(
        MeshId::Cube,
        worldTransform * Matrix4::translation(0.0f, 0.65f, 0.0f)
                       * Matrix4::scale(2.8f, 0.06f, 0.16f),
        {0.12f, 0.12f, 0.12f}
    );

    renderer.draw(
        MeshId::Cube,
        worldTransform * Matrix4::translation(0.0f, 0.65f, 0.0f)
                       * Matrix4::rotation(90.0f, {0.0f, 1.0f, 0.0f})
                       * Matrix4::scale(2.8f, 0.06f, 0.16f),
        {0.12f, 0.12f, 0.12f}
    );

    // Tail rotor sits beside the fin; both blades rotate in the XY plane.
    renderer.draw(
        MeshId::Cube,
        worldTransform * Matrix4::translation(-1.95f, 0.25f, 0.12f)
                       * Matrix4::scale(0.08f, 0.55f, 0.1f),
        {0.15f, 0.15f, 0.15f}
    );

    renderer.draw(
        MeshId::Cube,
        worldTransform * Matrix4::translation(-1.95f, 0.25f, 0.12f)
                       * Matrix4::rotation(90.0f, {0.0f, 0.0f, 1.0f})
                       * Matrix4::scale(0.08f, 0.55f, 0.1f),
        {0.15f, 0.15f, 0.15f}
    );

    // Landing skids
    for (int side = -1; side <= 1; side += 2)
    {
        // Vertical supports
        renderer.draw(
            MeshId::Cube,
            worldTransform * Matrix4::translation(-0.1f, -0.38f, side * 0.28f)
                           * Matrix4::scale(0.1f, 0.45f, 0.1f),
            {0.25f, 0.25f, 0.25f}
        );

        // Horizontal skid
        renderer.draw(
            MeshId::Cube,
            worldTransform * Matrix4::translation(-0.1f, -0.65f, side * 0.35f)
                           * Matrix4::scale(1.7f, 0.08f, 0.1f),
            {0.2f, 0.2f, 0.2f}
        );
    }
}
