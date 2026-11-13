#include "City.h"

#include "../renderer/Renderer.h"

namespace {
constexpr float chunkSize = 360.0f;
constexpr float cityHalfSize = 1.5f * chunkSize;
constexpr float roadSpacing = 120.0f;
constexpr float roadWidth = 8.0f;

void drawSurface(Renderer& renderer, float x, float z, float width, float depth,
                 float height, Color color)
{
    const Matrix4 model = Matrix4::translation(x, height, z)
                        * Matrix4::rotation(-90.0f, {1.0f, 0.0f, 0.0f})
                        * Matrix4::scale(width, depth, 1.0f);
    renderer.draw(MeshId::Quad, model, color);
}

void drawRoadLine(Renderer& renderer, float x, float z, float length,
                  bool alongZ, Color color)
{
    const Matrix4 model = Matrix4::translation(x, 0.08f, z)
                        * Matrix4::rotation(alongZ ? 90.0f : 0.0f, {0.0f, 1.0f, 0.0f})
                        * Matrix4::scale(length, 1.0f, 1.0f);
    renderer.draw(MeshId::Line, model, color);
}

void drawBox(Renderer& renderer, float x, float y, float z,
             float width, float height, float depth, Color color)
{
    const Matrix4 model = Matrix4::translation(x, y, z)
                        * Matrix4::scale(width, height, depth);
    renderer.draw(MeshId::Cube, model, color);
}

void drawDisc(Renderer& renderer, float y, float diameter, Color color)
{
    const Matrix4 model = Matrix4::translation(0.0f, y, 0.0f)
                        * Matrix4::rotation(-90.0f, {1.0f, 0.0f, 0.0f})
                        * Matrix4::scale(diameter, diameter, 1.0f);
    renderer.draw(MeshId::Disc, model, color);
}
}

void City::render(Renderer& renderer) const
{
    const Color ground = {0.85f, 0.83f, 0.77f};
    const Color asphalt = {0.27f, 0.29f, 0.32f};

    // Nine fixed chunks centered on the starting chunk; each shares its edges.
    for (int z = -1; z <= 1; ++z) {
        for (int x = -1; x <= 1; ++x) {
            drawSurface(renderer, x * chunkSize, z * chunkSize,
                        chunkSize, chunkSize, 0.0f, ground);
        }
    }

    // Draw the connected grid over the whole area. Boundary roads are half
    // width; the matching half would belong to the next chunk outside this view.
    constexpr int roadCount = 10;
    for (int i = 0; i < roadCount; ++i) {
        const float center = -cityHalfSize + i * roadSpacing;
        const bool edge = i == 0 || i == roadCount - 1;
        const float width = edge ? roadWidth / 2.0f : roadWidth;
        const float clippedCenter = center + (i == 0 ? roadWidth / 4.0f :
                                             i == roadCount - 1 ? -roadWidth / 4.0f : 0.0f);
        // North-south strips own the intersections.
        drawSurface(renderer, clippedCenter, 0.0f, width, 2.0f * cityHalfSize, 0.05f, asphalt);

        // East-west strips stop at the north-south strips to avoid overlap.
        for (int j = 0; j < roadCount - 1; ++j) {
            const float start = -cityHalfSize + j * roadSpacing + roadWidth / 2.0f;
            const float end = -cityHalfSize + (j + 1) * roadSpacing - roadWidth / 2.0f;
            drawSurface(renderer, (start + end) / 2.0f, clippedCenter,
                        end - start, width, 0.05f, asphalt);
        }
    }

    // Sidewalks outline each block and meet at their edges without overlap.
    const Color sidewalk = {0.72f, 0.70f, 0.64f};
    constexpr float sidewalkWidth = 3.0f;
    constexpr float blockWidth = roadSpacing - roadWidth;
    for (int z = 0; z < roadCount - 1; ++z) {
        for (int x = 0; x < roadCount - 1; ++x) {
            const float blockX = -cityHalfSize + (x + 0.5f) * roadSpacing;
            const float blockZ = -cityHalfSize + (z + 0.5f) * roadSpacing;
            const float offset = (blockWidth - sidewalkWidth) / 2.0f;
            drawSurface(renderer, blockX, blockZ - offset, blockWidth, sidewalkWidth, 0.15f, sidewalk);
            drawSurface(renderer, blockX, blockZ + offset, blockWidth, sidewalkWidth, 0.15f, sidewalk);
            drawSurface(renderer, blockX - offset, blockZ, sidewalkWidth, blockWidth - 2.0f * sidewalkWidth, 0.15f, sidewalk);
            drawSurface(renderer, blockX + offset, blockZ, sidewalkWidth, blockWidth - 2.0f * sidewalkWidth, 0.15f, sidewalk);
        }
    }

    // Center and stop markings use the shared GL_LINES mesh, slightly above asphalt.
    const Color yellow = {0.91f, 0.77f, 0.28f};
    const Color white = {0.95f, 0.95f, 0.95f};
    for (int i = 1; i < roadCount - 1; ++i) {
        const float center = -cityHalfSize + i * roadSpacing;
        for (int j = 0; j < roadCount - 1; ++j) {
            const float start = -cityHalfSize + j * roadSpacing + 6.0f;
            const float end = -cityHalfSize + (j + 1) * roadSpacing - 6.0f;
            const float middle = (start + end) / 2.0f;
            for (float offset : {-0.3f, 0.3f}) {
                drawRoadLine(renderer, middle, center + offset, end - start, false, yellow);
                drawRoadLine(renderer, center + offset, middle, end - start, true, yellow);
            }
            // A stop line spans the approaching lane before each intersection.
            drawRoadLine(renderer, start - 0.5f, center - 2.0f, 4.0f, true, white);
            drawRoadLine(renderer, end + 0.5f, center + 2.0f, 4.0f, true, white);
            drawRoadLine(renderer, center + 2.0f, start - 0.5f, 4.0f, false, white);
            drawRoadLine(renderer, center - 2.0f, end + 0.5f, 4.0f, false, white);
        }
    }

    // Starting tower: the shared cube is centered, so lift its center by
    // half its height to place its base on the ground.
    constexpr float towerHeight = 32.5f;
    const Matrix4 tower = Matrix4::translation(0.0f, towerHeight / 2.0f, 0.0f)
                        * Matrix4::scale(30.0f, towerHeight, 30.0f);
    renderer.draw(MeshId::Cube, tower, {0.36f, 0.48f, 0.60f});

    // Eight rows of fixed-size windows on all four facades, following the
    // reference tower's 3.5-unit floor height and 3-unit window spacing.
    const Color window = {0.66f, 0.86f, 0.94f};
    for (int floor = 0; floor < 8; ++floor) {
        const float y = 4.5f + floor * 3.5f + 0.6f + 1.1f;
        for (int bay = 0; bay < 9; ++bay) {
            const float along = (bay - 4) * 3.0f;
            for (float side : {-1.0f, 1.0f}) {
                drawBox(renderer, along, y, side * 15.02f, 2.0f, 2.2f, 0.02f, window);
                drawBox(renderer, side * 15.02f, y, along, 0.02f, 2.2f, 2.0f, window);
            }
        }
    }

    // Dark lobby, service entrances, and a projecting entrance canopy.
    const Color trim = {0.18f, 0.25f, 0.31f};
    drawBox(renderer, 0.0f, 1.5f, 15.02f, 3.6f, 3.0f, 0.02f, trim);
    drawBox(renderer, 0.0f, 3.45f, 15.6f, 5.6f, 0.3f, 1.2f, trim);
    drawBox(renderer, -7.5f, 1.2f, -15.02f, 1.8f, 2.4f, 0.02f, trim);
    drawBox(renderer, 15.02f, 1.2f, 0.0f, 0.02f, 2.4f, 1.6f, trim);
    drawBox(renderer, -15.02f, 1.1f, -11.3f, 0.02f, 2.2f, 1.4f, trim);

    // Four rooftop parapets and two boxes outside the landing disc.
    for (float side : {-1.0f, 1.0f}) {
        drawBox(renderer, 0.0f, towerHeight + 0.4f, side * 14.75f, 30.0f, 0.8f, 0.5f, trim);
        drawBox(renderer, side * 14.75f, towerHeight + 0.4f, 0.0f, 0.5f, 0.8f, 29.0f, trim);
    }
    drawBox(renderer, -11.0f, towerHeight + 1.25f, -11.5f, 6.0f, 2.5f, 4.0f,
            {0.54f, 0.58f, 0.63f});
    drawBox(renderer, 12.5f, towerHeight + 1.5f, 12.5f, 2.4f, 3.0f, 2.4f,
            {0.35f, 0.40f, 0.44f});

    // Layered 24-sided discs create the dark pad and yellow ring without lighting.
    drawDisc(renderer, towerHeight + 0.03f, 18.0f, {0.23f, 0.25f, 0.28f});
    drawDisc(renderer, towerHeight + 0.05f, 16.5f, {0.91f, 0.77f, 0.28f});
    drawDisc(renderer, towerHeight + 0.07f, 15.5f, {0.23f, 0.25f, 0.28f});
    const Color helipadMark = {0.95f, 0.95f, 0.95f};
    drawSurface(renderer, -2.0f, 0.0f, 1.0f, 6.0f, towerHeight + 0.09f, helipadMark);
    drawSurface(renderer, 2.0f, 0.0f, 1.0f, 6.0f, towerHeight + 0.09f, helipadMark);
    drawSurface(renderer, 0.0f, 0.0f, 3.0f, 1.0f, towerHeight + 0.09f, helipadMark);
}
