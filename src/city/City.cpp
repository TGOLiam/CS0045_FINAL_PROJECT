#include "City.h"

#include "../renderer/Renderer.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
constexpr float chunkSize = 360.0f;
constexpr float cityHalfSize = 1.5f * chunkSize;
constexpr float roadSpacing = 120.0f;
constexpr float roadWidth = 8.0f;

enum class BuildingStyle { Tower, Residential, Commercial };

struct Building {
    float x;
    float z;
    float width;
    float depth;
    float height;
    Color color;
    BuildingStyle style;
};

constexpr Color towerBlue = {93.0f/255.0f, 122.0f/255.0f, 153.0f/255.0f};
constexpr Color towerGreen = {106.0f/255.0f, 143.0f/255.0f, 143.0f/255.0f};
constexpr Color towerPurple = {125.0f/255.0f, 122.0f/255.0f, 160.0f/255.0f};
constexpr Color residentialClay = {201.0f/255.0f, 142.0f/255.0f, 107.0f/255.0f};
constexpr Color residentialSand = {212.0f/255.0f, 163.0f/255.0f, 115.0f/255.0f};
constexpr Color residentialRose = {181.0f/255.0f, 119.0f/255.0f, 110.0f/255.0f};
constexpr Color commercialYellow = {224.0f/255.0f, 201.0f/255.0f, 122.0f/255.0f};
constexpr Color commercialGreen = {199.0f/255.0f, 211.0f/255.0f, 160.0f/255.0f};
constexpr Color commercialPeach = {230.0f/255.0f, 160.0f/255.0f, 140.0f/255.0f};

// Starting chunk footprints from the design reference. The center tower is
// drawn separately, and the southeast block is reserved for the park.
constexpr Building startingBuildings[] = {
    {-144,-142,40,36,45.8f,residentialClay,BuildingStyle::Residential}, {-92,-146,36,28,14.7f,commercialYellow,BuildingStyle::Commercial},
    {-120,-92,70,30,11.3f,commercialGreen,BuildingStyle::Commercial}, {-25,-142,34,34,81.5f,towerBlue,BuildingStyle::Tower},
    {22,-144,40,36,39.4f,residentialSand,BuildingStyle::Residential}, {0,-90,80,24,11.3f,commercialPeach,BuildingStyle::Commercial},
    {95,-140,36,44,29.8f,residentialRose,BuildingStyle::Residential}, {144,-144,30,30,99.0f,towerGreen,BuildingStyle::Tower},
    {120,-88,60,22,7.9f,commercialYellow,BuildingStyle::Commercial}, {-142,0,36,70,11.3f,commercialPeach,BuildingStyle::Commercial},
    {-96,-22,36,36,42.6f,residentialClay,BuildingStyle::Residential}, {-96,24,36,36,26.6f,residentialSand,BuildingStyle::Residential},
    {-34,-36,24,20,7.9f,commercialGreen,BuildingStyle::Commercial}, {34,36,24,20,7.9f,commercialGreen,BuildingStyle::Commercial},
    {94,-22,36,40,52.2f,residentialRose,BuildingStyle::Residential}, {144,-24,36,36,14.7f,commercialYellow,BuildingStyle::Commercial},
    {120,30,46,28,71.0f,towerPurple,BuildingStyle::Tower}, {-144,98,32,32,88.5f,towerGreen,BuildingStyle::Tower},
    {-94,94,34,28,11.3f,commercialPeach,BuildingStyle::Commercial}, {-120,148,70,32,23.4f,residentialClay,BuildingStyle::Residential},
    {-26,94,36,30,14.7f,commercialGreen,BuildingStyle::Commercial}, {24,96,38,36,33.0f,residentialRose,BuildingStyle::Residential},
    {0,148,76,30,23.4f,residentialClay,BuildingStyle::Residential}
};

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

struct FacadeDetail {
    Matrix4 model;
    Color color;
};

std::vector<FacadeDetail> makeFacadeDetails()
{
    std::vector<FacadeDetail> details;
    for (const Building& b : startingBuildings) {
        const bool tower = b.style == BuildingStyle::Tower;
        const bool residential = b.style == BuildingStyle::Residential;
        const float groundHeight = residential ? 4.2f : 4.5f;
        const float floorHeight = tower ? 3.5f : (residential ? 3.2f : 3.4f);
        const float windowWidth = tower ? 2.0f : (residential ? 1.4f : 1.6f);
        const float windowHeight = tower ? 2.2f : (residential ? 1.6f : 1.5f);
        const float sill = tower ? 0.6f : 0.9f;
        const Color window = tower ? Color{0.66f, 0.86f, 0.94f}
                                   : Color{0.53f, 0.75f, 0.86f};
        const Color trim = tower ? Color{0.18f, 0.25f, 0.31f}
                                : (residential ? Color{0.54f, 0.42f, 0.33f}
                                               : Color{0.43f, 0.42f, 0.39f});
        for (int face = 0; face < 4; ++face) {
            const bool xFace = face >= 2;
            const float sign = (face % 2 == 0) ? 1.0f : -1.0f;
            const float span = xFace ? b.depth : b.width;
            const float pitch = tower ? 3.0f : (residential ? (xFace ? 6.0f : 4.0f) : 3.2f);
            const int bays = static_cast<int>(std::floor((span - 2.4f) / pitch));
            const int floors = static_cast<int>(std::round((b.height - groundHeight) / floorHeight));
            // The shared quad is XY-facing +Z; rotate it for the X walls.
            const Matrix4 wall = Matrix4::translation(
                b.x + (xFace ? sign * (b.width / 2.0f + 0.02f) : 0.0f),
                0.0f, b.z + (xFace ? 0.0f : sign * (b.depth / 2.0f + 0.02f)))
                * Matrix4::rotation(xFace ? 90.0f : 0.0f, {0.0f, 1.0f, 0.0f});
            auto add = [&](float u, float y, float width, float height, Color color) {
                details.push_back({wall * Matrix4::translation(u, y, 0.0f)
                                       * Matrix4::scale(width, height, 1.0f), color});
            };
            for (int floor = 0; floor < floors; ++floor) {
                const float y = groundHeight + floor * floorHeight + sill + windowHeight / 2.0f;
                if (y + windowHeight / 2.0f > b.height) continue;
                for (int bay = 0; bay < bays; ++bay) {
                    const float u = (bay - (bays - 1) / 2.0f) * pitch;
                    // Residential service side has a central stairwell strip.
                    if (residential && face == 3 && std::abs(u) < 1.2f) continue;
                    add(u, y, windowWidth, windowHeight, window);
                    if (residential && face == 0)
                        add(u, groundHeight + floor * floorHeight + 0.675f,
                            windowWidth + 0.8f, 0.25f, trim);
                }
            }
            const Color door = {0.14f, 0.15f, 0.18f};
            if (tower) {
                const float doorWidth = face == 0 ? 3.6f : (face == 1 ? 1.8f : (face == 2 ? 1.6f : 1.4f));
                add(0.0f, face == 0 ? 1.5f : 1.2f, doorWidth, face == 0 ? 3.0f : 2.4f, door);
                if (face == 0) add(0.0f, 3.45f, 5.6f, 0.3f, trim);
                if (face == 1) add(span * 0.2f, 1.4f, 3.0f, 1.2f, trim);
                if (face == 3) {
                    for (int louver = 0; louver < 3; ++louver)
                        add(0.0f, 0.95f + louver * 0.6f, span - 10.0f, 0.2f, trim);
                }
            } else if (residential) {
                const float doorWidth = face == 0 ? 2.4f : 1.2f;
                add(face == 3 ? span * 0.3f : 0.0f, 1.2f, doorWidth, 2.4f, door);
                if (face == 3) add(0.0f, (groundHeight + b.height) / 2.0f,
                                   1.0f, b.height - groundHeight, trim);
                if (face == 1) add(span * 0.2f, 1.5f, 1.6f, 0.6f, trim);
            } else {
                if (face == 0) {
                    add(-span / 2.0f + 1.9f, 1.3f, 1.8f, 2.6f, door);
                    // Shop glazing starts after the entrance and repeats in bays.
                    for (float u = -span / 2.0f + 4.8f; u + 1.5f < span / 2.0f - 1.0f; u += 3.2f)
                        add(u, 1.7f, 3.0f, 2.6f, window);
                    add(0.0f, 3.25f, span - 1.0f, 0.5f, {0.78f, 0.32f, 0.24f});
                    add(0.0f, 4.05f, span - 1.0f, 0.7f, trim);
                } else if (face == 1) {
                    add(-span * 0.2f, 1.2f, 1.6f, 2.4f, door);
                    add(span * 0.1f, 1.4f, 2.6f, 2.8f, door);
                } else if (face == 2) {
                    add(0.0f, 1.1f, 1.2f, 2.2f, door);
                }
            }
        }
    }
    return details;
}

void drawBuildingRoof(Renderer& renderer, const Building& b)
{
    const bool tower = b.style == BuildingStyle::Tower;
    const bool residential = b.style == BuildingStyle::Residential;
    const Color trim = tower ? Color{0.18f, 0.25f, 0.31f}
                            : (residential ? Color{0.54f, 0.42f, 0.33f}
                                           : Color{0.43f, 0.42f, 0.39f});
    // Four low parapets outline the roof without covering its surface.
    for (float side : {-1.0f, 1.0f}) {
        drawBox(renderer, b.x, b.height + 0.4f, b.z + side * (b.depth / 2.0f - 0.25f),
                b.width, 0.8f, 0.5f, trim);
        drawBox(renderer, b.x + side * (b.width / 2.0f - 0.25f), b.height + 0.4f, b.z,
                0.5f, 0.8f, b.depth - 1.0f, trim);
    }
    if (tower) {
        drawBox(renderer, b.x - b.width / 2.0f + 4.0f, b.height + 1.25f,
                b.z - b.depth / 2.0f + 3.5f, 6.0f, 2.5f, 4.0f, {0.54f, 0.58f, 0.63f});
        drawBox(renderer, b.x + b.width / 2.0f - 2.5f, b.height + 1.5f,
                b.z + b.depth / 2.0f - 2.5f, 2.4f, 3.0f, 2.4f, trim);
    } else if (residential) {
        drawBox(renderer, b.x, b.height + 1.2f, b.z - b.depth / 4.0f,
                3.0f, 2.4f, 3.0f, trim);
        for (float side : {-1.0f, 1.0f})
            drawBox(renderer, b.x + side * b.width / 4.0f, b.height + 1.2f,
                    b.z + b.depth / 4.0f, 1.0f, 2.4f, 1.0f, trim);
    } else {
        const int units = std::max(1, static_cast<int>(b.width / 12.0f));
        for (int i = 0; i < units; ++i)
            drawBox(renderer, b.x + (i - (units - 1) / 2.0f) * 6.0f,
                    b.height + 0.6f, b.z + b.depth / 4.0f, 2.0f, 1.2f, 1.5f, trim);
    }
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

    // Each building's center is half its height so its base rests at y=0.
    for (const Building& building : startingBuildings) {
        drawBox(renderer, building.x, building.height / 2.0f, building.z,
                building.width, building.height, building.depth, building.color);
        drawBuildingRoof(renderer, building);
    }

    // Generate the fixed facade transforms once; shared quad VBOs draw them.
    static const std::vector<FacadeDetail> facadeDetails = makeFacadeDetails();
    for (const FacadeDetail& detail : facadeDetails) {
        renderer.draw(MeshId::Quad, detail.model, detail.color);
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
