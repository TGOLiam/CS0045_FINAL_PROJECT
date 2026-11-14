#include "City.h"

#include "renderer/Renderer.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>
#include <vector>

namespace {
constexpr float chunkSize = CITY_CHUNK_SIZE;
constexpr float cityHalfSize = 1.5f * chunkSize;
constexpr float roadSpacing = CITY_ROAD_SPACING;
constexpr float roadWidth = CITY_ROAD_WIDTH;

bool chunkHasPark(int x, int z)
{
    // Keep the starting park and the same deterministic generated variants.
    return (x == 0 && z == 0) || ((x * 17 + z * 31) % 3 + 3) % 3 == 0;
}

template <typename Callback>
void forEachTree(Callback callback)
{
    // Local positions from the reference: mirrored park trees and street trees.
    const float parkPositions[][2] = {{14,14}, {36,12}, {12,36}, {24,24},
                                      {42,24}, {24,42}, {44,44}, {10,46}};
    const float streetPositions[][2] = {{-120,-66}, {0,-66}, {120,-66}, {-120,54},
                                        {120,54}, {-66,-120}, {-66,120}, {66,-120},
                                        {66,0}, {120,66}, {120,174}, {0,174}};
    for (int chunkZ = -1; chunkZ <= 1; ++chunkZ) {
        for (int chunkX = -1; chunkX <= 1; ++chunkX) {
            const float offsetX = chunkX * chunkSize;
            const float offsetZ = chunkZ * chunkSize;
            if (chunkHasPark(chunkX, chunkZ)) {
                for (float sx : {-1.0f, 1.0f}) {
                    for (float sz : {-1.0f, 1.0f}) {
                        for (const auto& p : parkPositions)
                            callback(offsetX + 120.0f + sx * p[0], offsetZ + 120.0f + sz * p[1]);
                    }
                }
            }
            for (const auto& p : streetPositions) callback(offsetX + p[0], offsetZ + p[1]);
        }
    }
}

template <typename Callback>
void forEachCar(Callback callback)
{
    struct CarPlacement { float x; float z; float yaw; int color; };
    // Cars face local +X; yaw places them in the reference's right-hand lanes.
    const CarPlacement placements[] = {
        {-90,-58,0,0}, {40,-62,180,1}, {-150,62,0,2}, {100,58,180,3},
        {-62,-100,270,4}, {-58,100,90,5}, {58,30,270,1}, {62,-30,90,0},
        {-30,62,0,3}, {130,58,180,2}, {178,-100,270,4}, {-178,40,90,5}
    };
    for (int chunkZ = -1; chunkZ <= 1; ++chunkZ) {
        for (int chunkX = -1; chunkX <= 1; ++chunkX) {
            for (const CarPlacement& car : placements)
                callback(car.x + chunkX * chunkSize, car.z + chunkZ * chunkSize,
                         car.yaw, car.color);
        }
    }
}

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
#include "city_layout_generated.h"

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

std::vector<FacadeDetail> makeFacadeDetails(const std::vector<Building>& buildings)
{
    std::vector<FacadeDetail> details;
    for (const Building& b : buildings) {
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

std::vector<Building> makeBuildings()
{
    std::vector<Building> buildings(std::begin(startingBuildings), std::end(startingBuildings));
    const Color towerColors[] = {towerBlue, towerGreen, towerPurple};
    const Color residentialColors[] = {residentialClay, residentialSand, residentialRose};
    const Color commercialColors[] = {commercialYellow, commercialGreen, commercialPeach};

    for (int chunkZ = -1; chunkZ <= 1; ++chunkZ) {
        for (int chunkX = -1; chunkX <= 1; ++chunkX) {
            if (chunkX == 0 && chunkZ == 0) continue;
            // Match the reference's deterministic chunk variants, including
            // its normalized modulo for negative coordinates.
            const int variant = ((chunkX * 17 + chunkZ * 31) % 3 + 3) % 3;
            const bool hasPark = chunkHasPark(chunkX, chunkZ);
            const float offsetX = chunkX * chunkSize;
            const float offsetZ = chunkZ * chunkSize;
            auto append = [&](Building b, int index) {
                const float ground = b.style == BuildingStyle::Residential ? 4.2f : 4.5f;
                const float floor = b.style == BuildingStyle::Tower ? 3.5f :
                                    (b.style == BuildingStyle::Residential ? 3.2f : 3.4f);
                const float desired = std::min(45.0f, b.height * (0.55f + variant * 0.1f));
                b.height = ground + std::max(1.0f, std::round((desired - ground) / floor)) * floor;
                const int colorIndex = (index + variant) % 3;
                b.color = b.style == BuildingStyle::Tower ? towerColors[colorIndex] :
                          (b.style == BuildingStyle::Residential ? residentialColors[colorIndex] : commercialColors[colorIndex]);
                b.x += offsetX;
                b.z += offsetZ;
                buildings.push_back(b);
            };
            append({0, 0, 30, 30, 25.0f, towerBlue, BuildingStyle::Tower}, 12);
            int index = 0;
            for (const Building& b : startingBuildings) {
                const int paletteIndex = index < 12 ? index : index + 1;
                append(b, paletteIndex);
                ++index;
            }
            if (!hasPark) {
                Building residential = {96, 100, 36, 40, 23.4f, residentialColors[variant], BuildingStyle::Residential};
                Building commercial = {144, 140, 32, 30, 11.3f, commercialColors[variant], BuildingStyle::Commercial};
                residential.x += offsetX;
                residential.z += offsetZ;
                commercial.x += offsetX;
                commercial.z += offsetZ;
                buildings.push_back(residential);
                buildings.push_back(commercial);
            }
        }
    }
    return buildings;
}

const std::vector<Building>& cityBuildings()
{
    static const std::vector<Building> buildings = makeBuildings();
    return buildings;
}

std::vector<Aabb> makeCollisionBounds()
{
    std::vector<Aabb> boxes;
    auto box = [&](float x, float y, float z, float w, float h, float d) {
        boxes.push_back({{x-w/2, y-h/2, z-d/2}, {x+w/2, y+h/2, z+d/2}});
    };
    for (const Building& b : cityBuildings()) {
        box(b.x, b.height/2, b.z, b.width, b.height, b.depth);
        for (float side : {-1.0f, 1.0f}) {
            box(b.x, b.height+0.4f, b.z+side*(b.depth/2-0.25f), b.width, 0.8f, 0.5f);
            box(b.x+side*(b.width/2-0.25f), b.height+0.4f, b.z, 0.5f, 0.8f, b.depth-1.0f);
        }
        if (b.style == BuildingStyle::Tower) {
            box(b.x-b.width/2+4, b.height+1.25f, b.z-b.depth/2+3.5f, 6, 2.5f, 4);
            box(b.x+b.width/2-2.5f, b.height+1.5f, b.z+b.depth/2-2.5f, 2.4f, 3, 2.4f);
        } else if (b.style == BuildingStyle::Residential) {
            box(b.x, b.height+1.2f, b.z-b.depth/4, 3, 2.4f, 3);
            for (float side : {-1.0f, 1.0f})
                box(b.x+side*b.width/4, b.height+1.2f, b.z+b.depth/4, 1, 2.4f, 1);
        } else {
            const int count = std::max(1, static_cast<int>(b.width/12));
            for (int i=0; i<count; ++i)
                box(b.x+(i-(count-1)/2.0f)*6, b.height+0.6f, b.z+b.depth/4, 2, 1.2f, 1.5f);
        }
    }
    // Central tower, parapets, canopy and rooftop equipment; include the
    // helipad markings' height in the roof collision surface.
    const float h = CITY_TOWER_HEIGHT;
    box(0, (h+0.09f)/2, 0, 30, h+0.09f, 30);
    for (float side : {-1.0f, 1.0f}) {
        box(0,h+0.4f,side*14.75f,30,0.8f,0.5f);
        box(side*14.75f,h+0.4f,0,0.5f,0.8f,29);
    }
    box(0,3.45f,15.6f,5.6f,0.3f,1.2f);
    box(-11,h+1.25f,-11.5f,6,2.5f,4);
    box(12.5f,h+1.5f,12.5f,2.4f,3,2.4f);
    // Trees use simple trunk collision; foliage remains visual geometry.
    forEachTree([&](float x, float z) { box(x, 1.0f, z, 0.7f, 2.0f, 0.7f); });
    // Static car collision boxes cover the body, cabin and wheels.
    forEachCar([&](float x, float z, float yaw, int) {
        const bool alongZ = yaw == 90.0f || yaw == 270.0f;
        box(x, 0.05f + 1.45f/2.0f, z, alongZ ? 1.84f : 4.4f,
            1.45f, alongZ ? 4.4f : 1.84f);
    });
    return boxes;
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

void drawDiscAt(Renderer& renderer, float x, float y, float z, float diameter, Color color)
{
    const Matrix4 model = Matrix4::translation(x, y, z)
                        * Matrix4::rotation(-90.0f, {1.0f, 0.0f, 0.0f})
                        * Matrix4::scale(diameter, diameter, 1.0f);
    renderer.draw(MeshId::Disc, model, color);
}

void drawDisc(Renderer& renderer, float y, float diameter, Color color)
{
    drawDiscAt(renderer, 0.0f, y, 0.0f, diameter, color);
}
}

std::vector<Aabb> City::collisionBounds() const
{
    return makeCollisionBounds();
}

bool City::initialize(Renderer& renderer)
{
    struct BatchData {
        Color color;
        std::vector<float> vertices;
    };
    // Group by chunk and solid color so the static facade quads can share draws.
    using BatchKey = std::tuple<int, int, int, int, int>;
    std::map<BatchKey, BatchData> groups;
    const auto details = makeFacadeDetails(cityBuildings());
    constexpr float quad[] = {
        -0.5f,-0.5f, 0.5f,-0.5f, 0.5f,0.5f,
        -0.5f,-0.5f, 0.5f,0.5f, -0.5f,0.5f
    };
    for (const FacadeDetail& detail : details) {
        const float* m = detail.model.data();
        const int chunkX = static_cast<int>(std::floor((m[12] + chunkSize / 2.0f) / chunkSize));
        const int chunkZ = static_cast<int>(std::floor((m[14] + chunkSize / 2.0f) / chunkSize));
        const BatchKey key = {chunkX, chunkZ,
            static_cast<int>(std::round(detail.color.r * 255.0f)),
            static_cast<int>(std::round(detail.color.g * 255.0f)),
            static_cast<int>(std::round(detail.color.b * 255.0f))};
        BatchData& batch = groups[key];
        batch.color = detail.color;
        for (int i = 0; i < 12; i += 2) {
            const float x = quad[i];
            const float y = quad[i + 1];
            batch.vertices.push_back(m[0] * x + m[4] * y + m[12]);
            batch.vertices.push_back(m[1] * x + m[5] * y + m[13]);
            batch.vertices.push_back(m[2] * x + m[6] * y + m[14]);
        }
    }
    facadeBatches_.clear();
    for (const auto& entry : groups) {
        std::size_t id = 0;
        if (!renderer.uploadStaticMesh(entry.second.vertices, GL_TRIANGLES, id)) return false;
        facadeBatches_.push_back({id, entry.second.color});
    }
    return true;
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

    // Park block from the reference: lawn, cross paths, a ring path, plaza,
    // and a pond. Small height offsets prevent coplanar depth flicker.
    constexpr float pi = 3.14159265358979323846f;
    const Color lawn = {127.0f/255.0f, 174.0f/255.0f, 107.0f/255.0f};
    const Color path = {234.0f/255.0f, 223.0f/255.0f, 191.0f/255.0f};
    const Color water = {127.0f/255.0f, 183.0f/255.0f, 217.0f/255.0f};
    for (int chunkZ = -1; chunkZ <= 1; ++chunkZ) {
        for (int chunkX = -1; chunkX <= 1; ++chunkX) {
            if (!chunkHasPark(chunkX, chunkZ)) continue;
            const float x = chunkX * chunkSize + 120.0f;
            const float z = chunkZ * chunkSize + 120.0f;
            drawSurface(renderer, x, z, 106.0f, 106.0f, 0.01f, lawn);
            drawSurface(renderer, x, z, 4.0f, 106.0f, 0.02f, path);
            drawSurface(renderer, x, z, 106.0f, 4.0f, 0.02f, path);
            // Twenty-four short quads approximate the circular 3-unit path.
            constexpr int segments = 24;
            constexpr float radius = 28.0f;
            const float segmentLength = 2.0f * radius * std::tan(pi / segments);
            for (int i = 0; i < segments; ++i) {
                const float angle = 2.0f * pi * i / segments;
                const Matrix4 ringSegment =
                    Matrix4::translation(x + radius * std::cos(angle), 0.03f,
                                         z + radius * std::sin(angle))
                    * Matrix4::rotation(-angle * 180.0f / pi, {0.0f, 1.0f, 0.0f})
                    * Matrix4::rotation(-90.0f, {1.0f, 0.0f, 0.0f})
                    * Matrix4::scale(3.0f, segmentLength, 1.0f);
                renderer.draw(MeshId::Quad, ringSegment, path);
            }
            drawDiscAt(renderer, x, 0.04f, z, 20.0f, path);
            drawDiscAt(renderer, x, 0.05f, z, 10.0f, water);
        }
    }

    // Trees reuse a cube trunk and the eight-sided cone mesh. The exporter
    // batches their solid-colored triangles into the static city asset.
    forEachTree([&](float x, float z) {
        drawBox(renderer, x, 1.0f, z, 0.7f, 2.0f, 0.7f, {0.42f, 0.29f, 0.18f});
        const Matrix4 lower = Matrix4::translation(x, 1.6f, z)
                            * Matrix4::scale(3.6f, 2.6f, 3.6f);
        const Matrix4 upper = Matrix4::translation(x, 3.4f, z)
                            * Matrix4::scale(2.4f, 2.2f, 2.4f);
        renderer.draw(MeshId::Cone, lower, {0.25f, 0.49f, 0.29f});
        renderer.draw(MeshId::Cone, upper, {0.31f, 0.60f, 0.35f});
    });

    // Static cars: one local model is positioned and rotated into each lane.
    const Color carColors[] = {{0.85f,0.31f,0.24f}, {0.18f,0.44f,0.71f},
                               {0.91f,0.77f,0.28f}, {0.94f,0.94f,0.94f},
                               {0.24f,0.56f,0.37f}, {0.17f,0.18f,0.21f}};
    const Color glass = {0.44f, 0.66f, 0.78f};
    const Color tires = {0.08f, 0.08f, 0.09f};
    forEachCar([&](float x, float z, float yaw, int color) {
        const Matrix4 car = Matrix4::translation(x, 0.05f, z)
                          * Matrix4::rotation(yaw, {0.0f, 1.0f, 0.0f});
        auto part = [&](MeshId mesh, float px, float py, float pz,
                        float width, float height, float depth, Color tint) {
            renderer.draw(mesh, car * Matrix4::translation(px, py, pz)
                                    * Matrix4::scale(width, height, depth), tint);
        };
        part(MeshId::Cube, 0, 0.6f, 0, 4.4f, 0.6f, 1.8f, carColors[color]);
        part(MeshId::Cube, -0.2f, 1.175f, 0, 2.3f, 0.55f, 1.6f, carColors[color]);
        for (float side : {-1.0f, 1.0f}) {
            part(MeshId::Cube, -0.7f, 1.18f, side * 0.81f, 1.0f, 0.4f, 0.02f, glass);
            part(MeshId::Cube, 0.4f, 1.18f, side * 0.81f, 0.9f, 0.4f, 0.02f, glass);
            for (float wheelX : {-1.4f, 1.4f})
                part(MeshId::Cylinder, wheelX, 0.32f, side * 0.81f, 0.64f, 0.64f, 0.22f, tires);
        }
        part(MeshId::Cube, 0.96f, 1.18f, 0, 0.02f, 0.4f, 1.4f, glass);
        part(MeshId::Cube, -1.36f, 1.18f, 0, 0.02f, 0.4f, 1.4f, glass);
    });

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

    // Construct fixed layouts once; each center is half its building's height.
    for (const Building& building : cityBuildings()) {
        drawBox(renderer, building.x, building.height / 2.0f, building.z,
                building.width, building.height, building.depth, building.color);
        drawBuildingRoof(renderer, building);
    }

    // Facade positions were baked and uploaded once during initialization.
    for (const FacadeBatch& batch : facadeBatches_) {
        renderer.drawStaticMesh(batch.meshId, Matrix4::identity(), batch.color);
    }

    // Starting tower: the shared cube is centered, so lift its center by
    // half its height to place its base on the ground.
    constexpr float towerHeight = CITY_TOWER_HEIGHT;
    const Matrix4 tower = Matrix4::translation(0.0f, towerHeight / 2.0f, 0.0f)
                        * Matrix4::scale(30.0f, towerHeight, 30.0f);
    renderer.draw(MeshId::Cube, tower, {0.36f, 0.48f, 0.60f});

    // Fixed-size windows on all four facades, with 3.5-unit floor spacing.
    const Color window = {0.66f, 0.86f, 0.94f};
    const int towerFloors = std::max(0, static_cast<int>(std::floor((towerHeight - 4.5f) / 3.5f)));
    for (int floor = 0; floor < towerFloors; ++floor) {
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
